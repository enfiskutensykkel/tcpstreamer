#!/bin/bash --

identity_file=/root/.ssh/id_dsa
project_name_format="%Y-%m-%d_%H%M"

usage(){
	# Give script usage
	case "$1" in
		"stop")
			printf "Usage: $0 $1 [-d] <output dir>\n"
			printf "  %2s\t%s\n" "-d" "Delete data files on hosts after retrieving them."
			;;
		"start")
			printf "Usage: $0 $1 [-n <prefix>] [-o <dir>] [-s] <filter file>...\n"
			printf "  %2s\t%s\n" "-n" "Prepend <prefix> to output files."
			printf "  %2s\t%s\n" "-o" "Use <dir> as output directory."
			printf "  %2s\t%s\n" "-s" "Synchronize host clocks first."
			printf "\nFilter file format:\n"
			printf "  <host>\t<iface>\t<filter string>\n"
			printf "\nA typical filter file looks like this:\n"
			printf "  172.16.0.10\teth1\ttcp\n"
			printf "  172.16.0.20\teth1\thost 10.0.0.10\n"
			printf "  172.16.0.30\teth1\ttcp and host 10.0.1.1\n"
			printf "  172.16.0.30\teth2\n"
			;;
		*)
			printf "Usage: $0 {start|stop} <args>...\n"
			printf "\nUse one of the following commands:\n"
			printf "    %-5s    %s\n" "start" "Start tcpdump instances on hosts."
			printf "    %-5s    %s\n" "stop" "Stop tcpdump instances on hosts."
			;;
	esac
	exit 1
}

remote_run(){
	printf "  %16s %6s " "$1" "$2"
	if [ "$5" -ne 0 ]; then
		(ssh -i $identity_file root@$1 "( ${@:6} 1>&2 )& echo \"$1\" \"\$!\"; wait \$!" 1>> $3 2>> $4)&
	else
		ssh -i $identity_file root@$1 "( ${@:6} )" 1>> $3 2>> $4
	fi
	
	status=$?
	if [ $status -eq 130 ]; then
		# assume ^C
		# FIXME: Make an abort routine
		printf "ABORTED\n"
		exit 1
	elif [ $status -ne 0 ]; then
		# something went wrong
		printf "[FAIL]\n"
	else
		# everything better than expected
		printf "[ OK ]\n"
	fi

	return $status
}

remote_copy(){
	printf "  %16s %6s " "$1" "$2"
	
	(scp -q -i $identity_file "root@$1:$3" "$4" &> /dev/null) && ( [ "$5" -eq 0 ] && (ssh -i $identity_file root@$1 "rm -f $3" &> /dev/null) || true )

	status=$?
	if [ $status -eq 130 ]; then
		# assume ^C
		# FIXME: Make an abort routine
		printf "ABORTED\n"
		exit 1
	elif [ $status -ne 0 ]; then
		# something went wrong
		printf "[FAIL]\n"
	else
		# everything better than expected
		printf "[ OK ]\n"
	fi

	return $status
}

tcpdump_start(){
	printf "Using prefix: $2\n"

	# Extract the hosts from the filter files
	hosts=()
	for file in ${@:4}; do
		while read line; do
			line=($line); host=${line[@]:0:1}
			
			i=0
			while (($i <= ${#hosts[@]})); do
				if [ "${hosts[$i]}" == "$host" ]; then
					break;
				fi

				let i+=1
			done

			if (($i > ${#hosts[@]})); then
				hosts=("${hosts[@]}" "$host")
			fi
			#unset i

		done < $file
	done

	# Synchronize host clocks
	if [ $3 -eq 1 ]; then
		printf "Synchronizing host clocks...\n"
		for host in ${hosts[@]}; do
			remote_run "$host" "" "/dev/null" "/dev/null" 0 "ntpdate -t 2 172.16.0.1"
		done
	fi

	# Start tcpdump on hosts
	printf "Starting tcpdump filters...\n"
	for file in ${@:4}; do
		while read line; do
			line=($line)
			host=${line[0]}
			iface=${line[1]}
			filter="${line[@]:2}"

			filename="/root/${2}_${host}_${iface}.pcap"

			# write filename to file list file
			echo "$host $iface $filename $filter" >> "$1/$2.list"

			# install filter
			remote_run "$host" "$iface" "$1/$2.pid" "$1/$2_${host}_${iface}.out" 1 "tcpdump -i $iface -Nn -w $filename \"$filter\""

		done < $file
	done

	printf "Output directory: $1\n"
}

tcpdump_stop(){
	printf "Using directory: $1\n"

	# Kill tcpdump processes
	files=(`find $1 -maxdepth 1 -type f -name "*.pid"`)
	if [ ${#files} -gt 0 ]; then

		# Loop through pid files in directory
		for file in ${files[@]}; do
			hosts=()
			pids=()

			# get pids to kill
			i=0
			while read line; do
				line=($line)
				hosts[$i]=${line[0]}
				pids[$i]=${line[1]}
				let i+=1
			done < $file
			count=$i

			# check if the processes are still running
			i=0
			while (( $i < $count )); do
				ps_count=$(ssh -i $identity_file root@${hosts[$i]} "ps ${pids[$i]} | grep -c \"tcpdump\"" 2> /dev/null)
				if [ $? -ne 0 ] || [ "$ps_count" -ne 1 ]; then
					unset hosts[$i] pids[$i]
				fi
				let i+=1
			done

			
			# kill tcpdump 
			if [ ${#pids[@]} -gt 0 ]; then
				printf "Killing tcpdump...\n"
				i=0
				while (( $i < $count )); do
					if [ ! -z "${hosts[$i]}" ] && [ ! -z "${pids[$i]}" ]; then
						remote_run "${hosts[$i]}" "${pids[$i]}" "/dev/null" "/dev/null" 0 "/bin/kill -15 ${pids[$i]}"
					fi
					let i+=1
				done
			else
				printf "tcpdump already killed.\n"
			fi
		done
	else
		printf "No tcpdump PIDs found.\n"
	fi

	# Retrieve data files
	files=(`find $1 -maxdepth 1 -type f -name "*.list"`)
	if [ ${#files} -gt 0 ]; then

		# Loop through list files in directory
		for file in ${files[@]}; do
			hosts=()
			ifaces=()
			remote_files=()

			# get the file list
			i=0
			while read line; do
				line=($line)
				hosts[$i]=${line[0]}
				ifaces[$i]=${line[1]}
				remote_files[$i]=${line[2]}
				let i+=1
			done < $file
			count=$i

			# retrieve pcap files
			name=${file%.list}
			printf "Retrieving data files...\n"
			i=0
			while (( $i < $count )); do
				remote_copy "${hosts[$i]}" "${ifaces[$i]}" "${remote_files[$i]}" "$1/${name##*/}_${hosts[$i]}_${ifaces[$i]}.pcap" "$2"
				let i+=1
			done
		done

	else
		printf "No data files to retrieve.\n"
	fi
}

if [ $# -eq 1 ]; then usage "$*"; fi
case "$1" in

	"start")
		project_name=`date "+${project_name_format}"`

		while getopts ":n:o:s" opt ${@:2}; do
			case "$opt" in
				"n") project_name=$OPTARG;;
				"o") output_dir=$OPTARG;;
				"s") sync_clocks=1;;
				"?") echo "Invalid option: -$OPTARG"; exit 1;;
				":") echo "Option -$OPTARG requires an argument."; exit 1;;
			esac
			skip=$((OPTIND))
		done
		shift $skip
		
		if [ ${#@} -eq 0 ]; then usage "start"; fi

		for file in "$@"; do
			if [ ! -f "$file" ] || [ ! -r "$file" ]; then
				echo "Not a readable file: $file"
				exit 1
			fi
		done

		if [ ! -e "${output_dir=./$project_name}" ]; then
			mkdir "$output_dir"
			if [ $? -ne 0 ]; then
				echo "Unexpected error, unable to create output directory."
				exit 1
			fi
		elif [ ! -d "$output_dir" ] || [ ! -w "$output_dir" ]; then
			echo "Not a writeable directory: ${output_dir:-.}"
			exit 1
		fi

		for file in {"$project_name.list","$project_name.pid"}; do
			if [ -e $output_dir/$file ]; then
				echo "Unexpected error: $output_dir/$file already exists."
				exit 1
			fi

			touch "$output_dir/$file"
			if [ $? -ne 0 ]; then
				echo "Unexpected error, unable to create output directory."
				exit 1
			fi
		done

		tcpdump_start "$output_dir" "$project_name" "${sync_clocks:-0}" "$@"
		;;

	"stop")
		carefulness=1
		while getopts ":d" opt ${@:2}; do
			case "$opt" in
				"d") carefulness=0;;
				"?") echo "Invalid option: -$OPTARG"; exit 1;;
				":") echo "Option $OPTARG requires an argument."; exit 1;;
			esac
			skip=$((OPTIND))
		done
		shift $skip

		if [ ${#@} -eq 0 ]; then usage "stop"; fi

		for dir in $@; do
			if [ ! -d "$dir" ] || [ ! -w "$dir" ]; then
				echo "Not a writeable directory: $dir"
				exit 1
			fi
		done

		for dir in $@; do
			tcpdump_stop "$dir" "$carefulness"
		done
		;;

	*) 
		# Errornous command given
		usage "$*"
		;;

esac
exit 0

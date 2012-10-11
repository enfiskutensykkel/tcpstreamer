#!/bin/bash
if [ "$#" -lt 3 ]; then
	exit 1
fi

funcs=()
for file in ${@:3}; do
	# FIXME Find first function declaration instead
	func=`basename ${file} .c`
	funcs=("${funcs[@]}" "${func}")
done 

for func in ${funcs[@]}; do
	echo "extern streamer_t ${func};"
done >> $1

echo "streamer_t $2[] = {" >> $1
for func in ${funcs[@]}; do
	echo "&${func},"
done >> $1
echo "0};" >> $1

exit 0

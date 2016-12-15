#!/bin/sh

logdir="log"
srcdir="src"
buildfiledir="../bin/"
elevatornumber=4
if [ -d $buildfiledir ];then
	echo
else
	mkdir $buildfiledir
fi

touchlogfiles(){
	cd $logdir
	for x in 0 1 2 3
	do
		filename="elevator_$x.log"
		touch $filename
	done
	cd ../
}
if [ -d $logdir ]; then
	touchlogfiles
else
	mkdir $logdir
	touchlogfiles
fi


if [ "$1" = "-g" ];then
	isdebug="-g"
else
	isdebug=""
fi

cd $srcdir
echo "Enter src directory"
 # elevator
 gcc -Wall $isdebug -c $(ls elevator*.c)
 gcc -Wall $isdebug -c "priority_queue.c"
 gcc -Wall $isdebug -o "elevator" $(ls elevator*.o) "priority_queue.o" -lpthread

# controller
gcc -Wall $isdebug -c $(ls controller*.c)
#gcc -Wall $isdebug -c "priority_queue.c"
gcc -Wall $isdebug -o "controller" $(ls controller*.o) "priority_queue.o" -lpthread

# move
mv "elevator" $buildfiledir
mv "controller" $buildfiledir

cd ../
echo "Leaved src directory"
exit 0

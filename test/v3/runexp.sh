

for FILE in ../testdata/*
do
   #echo $FILE
   name=$( echo $FILE | awk -F/ 'OFS=FS {print $3}' )
   echo $name
	for var in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
	do
		b=$( { /usr/bin/time -f "%e" ./WL $var < $FILE > out$name; } 2>&1 )
		echo $var $b
	done
done
 


	#RES=$(echo $b | awk -Freal 'OFS=FS {print $2}' | awk -Fs 'OFS=FS {print $1}')
	#echo $var $RES

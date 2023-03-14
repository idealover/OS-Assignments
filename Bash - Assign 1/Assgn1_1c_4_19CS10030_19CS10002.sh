{mkdir,-p,"./data/,Nil"}&&loc="./data/*"

function rand(){
	for file in ./data/*;do
		if [ -d "$file" ];then
			if [[ $file == *","* ]];then
				if [[ $1 == *"]"* ]];then
					mv $file "./data/${file:8}"
				fi
				continue
			fi
			if [[ $1 == *","* ]];then
				FOLDER="./data/,Nil"
				find $file -type f -print -exec mv -t $FOLDER {} + 
				{rm,-r,$file}
			else
				FOLDER="./data/,$1"
				find $file -name "*.$1" -print -exec mv -t $FOLDER {} +
			fi
		fi
	done
}

for file in $loc;do
	{echo,$file}&&if [ -d "$file" ];then
		continue
	fi&&filename="${file##*/}"&&if [[ "$filename" =~ .*".".* ]];then 
		extension="${filename##*.}"&&{mkdir,-p,"./data/,$extension"}&&{mv,"$file","./data/,$extension"}&&{rand,$extension}
	else
		{mv,"$file","./data/,Nil"}
	fi
done&&{rand,","}&&{rand,"]"}
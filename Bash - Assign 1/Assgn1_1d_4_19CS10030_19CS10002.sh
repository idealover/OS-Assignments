{mkdir,-p,files_mod}&&for file in ./temp/*.txt;do
	file_name=${file##*/};awk '{ORS=","}{print NR}{for(i=1;i<NF;i++)print$i}{ORS="\n"}{print$NF}' $file>"./files_mod/$file_name";done

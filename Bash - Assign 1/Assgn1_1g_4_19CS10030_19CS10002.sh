space=${IFS%??}&&{printf,"Name${space}of${space}file:${space}"}&&{read,NAME}&&>$NAME&&for((i=0;i<150;i+=1));do
	entries=($(shuf${space}-i${space}0-15000${space}-n${space}10))&&printf${space}'%s\n'${space}${entries[*]}|paste${space}-sd','>>$NAME
done&&{printf,"Column${space}Number:${space}"}&&{read,COL}&&{printf,"Regular${space}Expression:${space}"}&&{read,EXP}&&while IFS=',' read${space}-r${space}line
do
	[[ $line =~ $EXP ]]&&{echo,YES}&&{exit,0}
done< <(cut${space}-d","${space}-f"${COL}"${space}$NAME|tail${space}-n${space}+2)
{echo,NO}
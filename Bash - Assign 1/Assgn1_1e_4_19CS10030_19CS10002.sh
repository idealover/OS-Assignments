>invalid.txt&&>valid.txt&&n="-s $1";{export,REQ_HEADERS="Connection,Keep-Alive"};{curl,$n,"https://www.example.com/"}>example.html;{curl,$n,"http://ip.jsontest.com/"}|{jq,-r,'.ip'};{curl,$n,"http://headers.jsontest.com/"}|{jq,-r,".\"${REQ_HEADERS//,/\",.\"}\""};for FILE in jsondata/*.json;do
	DATA=$({cat,"$FILE"}|{tr,-d,'[:space:]'})&&BASENAME=$(basename ${FILE})
	ISVALID=$({curl,$n,"http:/validate.jsontest.com/",-G,-d"json=$DATA"}|{jq,-r,'.validate'})
	[[ $ISVALID == true ]]&&printf "$BASENAME\n">>valid.txt||printf "$BASENAME\n">>invalid.txt
done
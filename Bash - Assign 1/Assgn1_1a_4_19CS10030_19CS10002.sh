n=$1&&while(($((n%2))==0));do
	{printf,"2${IFS%??}"}&&n=$((n/2))
done&&for((i=3;$((i*i))<=n;i+=2));do
	while(($((n%i))==0));do
		{printf,"${i}${IFS%??}"}&&n=$((n/i))
	done
done&&((n>2))&&{printf,"${n}\n"}
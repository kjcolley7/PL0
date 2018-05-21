var n;

procedure IsEven(n);
	if n = 0 then return := 1
	else return := call IsOdd(-n + 1);

procedure IsOdd(n);
	if n = 0 then return := 0
	else return := call IsEven(-n - 1);

begin
	read n;
	write call IsEven(n);
end.

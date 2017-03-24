var n, fact;

procedure Factorial(n);
begin
	if n > 1 then
		return := n * call Factorial(n - 1)
	else
		return := 1
end;

begin
	read n;
	fact := call Factorial(n);
	write fact;
end.

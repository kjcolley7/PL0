var n, fact;

procedure Factorial(n);
begin
	if n > 0 then begin
		return := n * call Factorial(n - 1);
	end
	;/*else begin*/
	if n <= 0 then begin
		return := 1;
	end;
end;

begin
	read n;
	fact := call Factorial(n);
	write fact;
end.

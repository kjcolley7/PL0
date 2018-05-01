var a, b;

procedure gcd(a, b);
	begin
		if b = 0
			then return := a
			else return := call gcd(b, a % b)
	end;

begin
	read a;
	read b;
	write call gcd(a, b);
end.

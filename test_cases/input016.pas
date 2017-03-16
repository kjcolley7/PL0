var n, fib;

procedure Fibonacci(n);
begin
	if n > 1 then begin
		/*{ Naive, slow, recursive method }*/
		return := call Fibonacci(n - 1) + call Fibonacci(n - 2);
	end
	else begin
		return := n;
	end;
end;

begin
	read n;
	fib := call Fibonacci(n);
	write fib;
end.

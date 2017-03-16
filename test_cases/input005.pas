const a = 1, b = 100, MAX=99999, ZERO = 0, ONE = 1;
var c, d, e;
procedure main();
	const g = 42;
	var h, fibParam, fibResult;
	procedure fib();
		var a, b, tmp, i, n;
		begin
			n := fibParam;
			a := 0;
			b := 1;
			
			if n <= 1 then
				fibResult := n
			else
			begin
				i := 2;
				while i <= n do
				begin
					write a;
					tmp := a + b;
					a := b;
					b := tmp;
					i := i + 1
				end;
				
				fibResult := a
			end
		end;
	
	begin
		h := -g + b;
		c := h;
		
		if odd c then write ONE
		else write ZERO;
		
		while h >= 0 do
		begin
			write h;
			h := h - 1
		end;
		
		fibParam := 47;
		call fib;
		write fibResult
	end;
call main.

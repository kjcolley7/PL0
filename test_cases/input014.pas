procedure BoxVolume(l, w, h);
	begin
		/*{ Ensure we can return a value from a procedure }*/
		return := l * w * h;
	end;

procedure WriteVolume(l, w, h);
	begin
		/*{ Ensure we can use "return" like a normal variable }*/
		return := call BoxVolume(l, w, h);
		write return;
	end;

procedure Main();
	const L = 7, W = 11, H = 13;
	var volume;
	begin
		/*{ Ensure we can call a procedure with parameters as a statement }*/
		call WriteVolume(2, 4, 7);
		
		/*{ Ensure we can use the return value of a procedure call with parameters }*/
		volume := call BoxVolume(L, W, H);
		write volume;
		
		/*{ Ensure we can use the return value of a procedure call in a condition }*/
		if 2 * call BoxVolume(L, W, H) > 2000 then begin
			/*{ Ensure we can use expressions as parameter values }*/
			call WriteVolume(2 * L, W, H);
		end
		;/*else begin*/
		if 2 * volume <= 2000 then begin
			call WriteVolume(2000, 1, 1);
		end;
		
		/*{ Ensure we can use the result of a procedure call within a parameter for another procedure call }*/
		call WriteVolume(2, 3, 3 * call BoxVolume(3, 5, 4) - 173);
	end;

/*{ Ensure we can still call a procedure without parameters without parentheses as a statement }*/
call Main.

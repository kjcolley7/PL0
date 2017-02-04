const ZERO=0, ONE=1;

procedure A();
	var x;
	begin
		x := 0;
		
		/*{ All three code blocks should branch directly to write ONE }*/
		if x <> 0 then begin
			write ZERO
		end
		else begin
			if x > 1 then begin
				call A
			end
			else begin
				write x
			end
		end;
		
		write ONE;
		
		/*{ The next two conditions should disappear from mcode.txt }*/
		if x > 0 then;
		if x > 1 then else;
		if x > 2 then else write x;
		
		write ONE;
		
		/*{ No code should be generated here }*/
		;;;;;;;;;;
		
		write ONE;
		
		/*{ No code should be generated here }*/
		begin begin begin end;;; begin end end end;
		
		/*{ All three statements should be followed directly by RETs }*/
		if x <> 0 then begin
			if x > 1 then begin
				call A
			end
			else begin
				write x
			end
		end
		else begin
			write ZERO
		end;;;;;;
	end;

/*{ main }*/
begin
	call A;
	
	/*{ Both write instructions should be followed directly by HALTs }*/
	if ZERO < ONE then begin
		write ZERO
	end
	else begin
		write ONE
	end
end.

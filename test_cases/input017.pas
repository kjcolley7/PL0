var result;

/*{ Should this be legal? The assignment description doesn't mention either way. }*/
procedure return(a, b, c);
	begin
		return := a + b + c;
		write return;
	end;

begin
	result := call return(1, 2, 3) + 42;
	write result;
	
	if call return(1, 2, 3) = 6 then /*else*/;
	
	result := result - call return(call return(1, 0, 0), call return(0, 2, 0), call return(0, 0, 3));
	write result;
end.

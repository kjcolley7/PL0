
procedure tac();
	var c;
	begin
		read c;
		if c <> -1 then begin
			call tac;
			write c;
		end;
	end;

begin
	call tac;
end.

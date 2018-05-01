
procedure tac();
	var a,b,c,d,e,f;
	begin
		read a;
		if a <> -1 then begin
			read b;
			if b <> -1 then begin
				read c;
				if c <> -1 then begin
					read d;
					if d <> -1 then begin
						read e;
						if e <> -1 then begin
							read f;
							if f <> -1 then begin
								call tac;
								write f;
							end;
							write e;
						end;
						write d;
					end;
					write c;
				end;
				write b;
			end;
			write a;
		end;
	end;

begin
	call tac;
end.

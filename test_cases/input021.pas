var inMe, id;

procedure Me();
	var id;
	
	procedure Mom();
		var id;
		
		procedure MomsMom();
			var id;
			begin
				id := 1;
				write id;
				call MomsDad;
			end;
		
		procedure MomsDad();
			var id;
			begin
				id := 2;
				write id;
				call Dad;
			end;
		
		begin
			id := 3;
			write id;
			call MomsMom;
			call Dad;
		end;
	
	procedure Dad();
		var id;
		
		procedure DadsMom();
			var id;
			begin
				id := 4;
				write id;
				call DadsDad;
			end;
		
		procedure DadsDad();
			var id;
			begin
				id := 5;
				write id;
				call Friend;
			end;
		
		begin
			id := 6;
			write id;
			call DadsMom;
		end;
	
	begin
		id := 7;
		write id;
		call Mom;
	end;

procedure Friend();
	var id;
	begin
		id := 8;
		write id;
		if inMe = 0 then begin
			inMe := 1;
			call Me;
			inMe := 0;
		end;
	end;

begin
	id := 9;
	write id;
	inMe := 0;
	call Friend;
end.

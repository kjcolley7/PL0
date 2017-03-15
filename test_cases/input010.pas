const ONE=1;

procedure EMPTY();;

begin
	call EMPTY;
	call EMPTY;
	write ONE;
	call EMPTY;
	write ONE;
	
	/*
	if 1 < 2 then
		call EMPTY
	else if 3 < 4 then
		call EMPTY
	else
		call EMPTY;
	*/
	
	if 1 < 2 then
		call EMPTY
	;
	if 1 >= 2 then begin
		if 3 < 4 then
			call EMPTY
		;
		if 3 >= 4 then
			call EMPTY
	
	call EMPTY
end.
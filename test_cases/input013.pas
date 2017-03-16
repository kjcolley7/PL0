const beginningX= 3;

var facRes, facParam;

procedure Factorial();
var myFacParam;
begin
	if facParam <= 0 then facRes := 1;
	if facParam > 0 then begin
		myFacParam:= facParam;
		facParam:= facParam*2/2 -1*1;
		
		call Factorial;
		
		if myFacParam <> facParam +1 then /*{This should never be taken}*/
		begin
			facParam:=0;
		end;
		
		
		facRes:= facRes * (facParam+1);
		facParam:= myFacParam;
	end
end;

begin
	facParam:= beginningX; /*{Just to have done something with a constant as well}*/
	call Factorial;
	write facRes
end.

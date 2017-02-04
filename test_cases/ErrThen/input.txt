const z = 4;
var x, y;
begin
	y := 3;
	if y < 0 begin /* Missing 'then' before 'begin' */
		x := y + z;
	end
end.

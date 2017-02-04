const m = 7, n = 85;
var i, x, y, z, q, r;
procedure mult();
	var q, r;	/*{Modification: procedure mult should use variables q and r from its own scope}*/
	begin
		q := x; r := y; z := 0;
		while r > 0 do
		begin
			if odd r then z := z + q;
			q := 2 * q;
			r := r / 2;
		end
	end;
begin
x := m;
y := n;
call mult;
write z;
end.

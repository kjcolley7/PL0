const n = 13; /* constant declaration */
var i, h; /* variable declaration */

procedure sub();
	const k = 7;
	var j, h;
	begin
		j := n;
		i := 1;
		h := k;
	end;

begin /* main starts here */
	i := 3;
	h := 0;
	call sub;
end.

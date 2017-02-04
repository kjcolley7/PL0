const ZERO=0, ONE=1;
var x;

/*{
 SLOW isn't called, so it won't be generated in the code output in mcode.txt, but it
 will still have code generated for it internally and will be checked semantically.
 }*/
procedure SLOW();
	var n;
	procedure ADDRUND();
		begin
			call WriteOne;
			n := 1;
			write ZERO
		end;
	procedure Empty();;
	begin
		/*{ If this code were called it would take a long time to execute }*/
		n := 99999;
		while n > 0 do
		begin
			n := n - 1;
			write n
		end;
		write ZERO;
		call ADDRUND;
		call Empty;
		call Empty
	end;

/*{ WriteOne is called multiple times to make sure symbol resolution works }*/
procedure WriteOne();
	write ONE;

/*{ main }*/
begin
	call WriteOne;
	write ONE;
	call WriteOne;
	call WriteOne;
	read x;
	x := x + 4;
	write x;
	call WriteOne;
	write ZERO;
	call WriteOne
end.

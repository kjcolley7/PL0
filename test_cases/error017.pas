/*{ program MAIN_2; }*/

var X;
procedure BIGSUB();
	var A, B, C;
	
	procedure SUB1();
		var A, D;
		begin /*{ SUB1 }*/
			A := 1;
			D := 1;
			A := B + D;
		end; /*{ SUB1 }*/
	
	procedure SUB2(X);
		var B, E;
		
		procedure SUB3();
			var C, E;
			begin /*{ SUB3 }*/
				C := 1;
				E := 1;
				call SUB1;
				E := D + A;
			end; /*{ SUB3 }*/
		
		begin /*{ SUB2 }*/
			B := 1;
			E := 1;
			call SUB3;
			A := C + D;
		end; /*{ SUB2 }*/
	
	begin /*{ BIGSUB }*/
		A := 1;
		B := 1;
		C := 1;
		call SUB2(7);
	end; /*{ BIGSUB }*/

begin /*{ MAIN_2 }*/
	call BIGSUB;
end. /*{ MAIN_2 }*/

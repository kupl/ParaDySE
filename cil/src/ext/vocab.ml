open Cil

module E = Errormsg

let debug = ref false

let vinfo_from_exp (e: exp) : varinfo =
	match e with
	| Lval (Var vi, _) -> vi
	| _ -> invalid_arg "vi_from_exp"

let int_from_exp (e: exp) : int = 
	match e with
	| Const (CInt64 (i, _, _)) -> i64_to_int i
	| _ -> invalid_arg "int_from_exp"

let is_if_stmt (s: stmt) : bool =
	match s.skind with
	| If _ -> true
	| _ -> false

let first_stmt_of_fd (fd: fundec) : stmt =
	let stmts = fd.sbody.bstmts in
	assert (List.length stmts > 0);
	List.hd stmts

let first_stmt_of_blk (blk: block) : stmt =
	let stmts = blk.bstmts in
	assert (List.length stmts > 0);
	List.hd stmts

let get_branch_info (i: instr) : (int * bool) option =
	match i with
	| Call (_, (Lval (Var vi, _)), args, _) when vi.vname = "__CrestBranch" -> 
			assert (List.length args == 3);
			let bid = List.nth args 1 in
			let tf = List.nth args 2 |> int_from_exp |> function 1 -> true | _ -> false in
			if !debug then E.log "bid: %a, instr %a\n" d_exp bid d_instr i;
			Some (int_from_exp bid, tf)
	| _ -> None

let queue_to_list : 'a Queue.t -> 'a list 
= fun queue -> 
  Queue.fold (fun b_l element -> element::b_l) [] queue


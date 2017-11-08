open Cil
open Vocab
open Printf
module E = Errormsg
module D = Dominators

type bid = int

type feature = {
	is_in_main_func: bid list; (* #1 static feature *)
	is_Tbranch_loop: bid list;
	is_Fbranch_loop: bid list;
	is_nested: bid list;
	has_extern_in_cond: bid list;
	has_int_in_cond: bid list;
	has_constant_strings_in_cond: bid list;
	has_pointer_in_cond: bid list;
	has_local_variable_in_cond: bid list;
	is_in_loopbody: bid list;
	is_Tbranch_casestatement: bid list; 
	is_Fbranch_casestatement: bid list; (* #12 static feature *) 
}

let empty_feature = {
	is_in_main_func = [];
	is_Tbranch_loop = [];
	is_Fbranch_loop = [];
	is_nested = [];
	has_extern_in_cond = [];
	has_int_in_cond = [];
	has_constant_strings_in_cond = [];
	has_pointer_in_cond = [];
	has_local_variable_in_cond = [];
	is_in_loopbody = [];
	is_Tbranch_casestatement = []; 
	is_Fbranch_casestatement = []; 
}

let prerr_feature : feature -> unit
= fun feature ->
  let bidlist2str : bid list -> string
  = fun bidlist ->
    List.fold_right (fun i str -> str ^ " " ^ (string_of_int i)) bidlist "" in
     prerr_endline "== static features for Branch ==";
(*1*)prerr_endline ("is in main function? : " ^ bidlist2str feature.is_in_main_func);
     prerr_endline ("is loop's true cond ? : " ^ bidlist2str feature.is_Tbranch_loop);
     prerr_endline ("is loop's false cond ? : " ^ bidlist2str feature.is_Fbranch_loop);
     prerr_endline ("is nested? : " ^ bidlist2str feature.is_nested);
(*5*)prerr_endline ("has extern function in cond ? : " ^ bidlist2str feature.has_extern_in_cond);
     prerr_endline ("has integer in cond ? : " ^ bidlist2str feature.has_int_in_cond);
     prerr_endline ("has constant strings in cond ? : " ^ bidlist2str feature.has_constant_strings_in_cond);
     prerr_endline ("has pointer in cond? : " ^ bidlist2str feature.has_pointer_in_cond);
     prerr_endline ("has local var in cond? : " ^ bidlist2str feature.has_local_variable_in_cond);
(*0*)prerr_endline ("is branch in loop body? : " ^ bidlist2str feature.is_in_loopbody);
     prerr_endline ("is case's true branch? : " ^ bidlist2str feature.is_Tbranch_casestatement);
     prerr_endline ("is case's false branch? : " ^ bidlist2str feature.is_Fbranch_casestatement)

class virtual collectBranchVisitor = object(self)
	inherit nopCilVisitor as super

	val mutable visited : bool = false;
	val mutable bids = Queue.create ()
	method get : 'a Queue.t =
		if visited then bids else raise (Failure "Get request on queue before visiting")

	method push (bid: bid) : unit = Queue.push bid bids

	method print = 
		self#describe;
		Queue.iter (fun bid -> prerr_endline (string_of_int bid)) bids
	
	method virtual extract_features : feature -> feature

	method virtual describe : unit

	method vinst (i: instr) =
		match get_branch_info i with
		| Some (bid, _) ->
				self#push bid;
				SkipChildren
		| _ -> SkipChildren
	
	method vglob (g: global) = visited <- true; super#vglob g
	method vblock (b: block) = visited <- true; super#vblock b
end

class collectAllBranchVisitor = object(self)
	inherit collectBranchVisitor

	method extract_features (feat: feature) = feat
	method describe : unit = 
		prerr_endline "collectAllBranchVisitor"
end

class mainBranchVisitor = object(self)
	inherit collectBranchVisitor as super
	
	method extract_features (feat: feature) : feature =
		{ feat with is_in_main_func = super#get |> queue_to_list }

	method describe : unit =
		prerr_endline "mainBranchVisitor"
	
	method vfunc (fd: fundec) =
		if fd.svar.vname = "main"
		then DoChildren
		else SkipChildren
end

class loopCondVisitor (pred: bool) = object(self)
	inherit collectBranchVisitor as super

	val mutable is_inside_loop : bool = false
	val mutable is_found : bool = false

	method extract_features (feat: feature) : feature =
		match pred with
		| true -> { feat with is_Tbranch_loop = super#get |> queue_to_list }
		| false -> { feat with is_Fbranch_loop = super#get |> queue_to_list }

	method describe : unit =
		prerr_endline ("loopCondVisitor_" ^ (string_of_bool pred))

	method vinst (i: instr) =
		match get_branch_info i with
		| Some (bid, tf) when (not is_found) && tf = pred && is_inside_loop =true ->
				super#push bid;
				is_found <- true;
				SkipChildren
		| _ -> SkipChildren
	
	(*	Do not worry about nested loops since a loop branch appears always as
			the first stmt of a loop block
	*)
	method vstmt (s: stmt) =
		match s.skind with
		| Loop _ -> 
				is_inside_loop <- true;
				is_found <- false;
				DoChildren
		| If _ when is_inside_loop ->
				DoChildren
		| _ -> DoChildren
end

class nestedBranchVisitor = object(self)
	inherit collectBranchVisitor as super

	val mutable is_outside : bool = true
	val mutable is_checked : bool = false
	val mutable next_sid : int = 0

	method extract_features (feat: feature) : feature =
		{ feat with is_nested = super#get |> queue_to_list }

	method describe : unit =
		prerr_endline "nestedBranchVisitor"

	method vinst (i: instr) =
		match get_branch_info i with
		| Some (bid, _) when (not is_checked) ->
				super#push bid;
				is_checked <- true;
				SkipChildren
		| _ -> SkipChildren

	method vstmt (s: stmt) =
		(if s.sid = next_sid
		then (is_outside <- true; is_checked <- false)
		else ());
		match s.skind with
		| If (_, _, fb, _) when is_outside ->
				let fid = (first_stmt_of_blk fb).sid in
				is_outside <- false;
				is_checked <- false;
				next_sid <- fid;
				DoChildren
		| _ -> DoChildren
end	

class collectExternFuncNames = object(self)
	inherit nopCilVisitor
	val mutable ext_vals = Queue.create ()
	method get = ext_vals

	method vinst (i: instr) =
		match i with
		| Call((Some (Var vi, _)), (Lval (Var func_name,_)), _, _) 
		  when (func_name.vstorage = Extern) && (vi.vdescr != Pretty.nil)-> 
		 		Queue.push vi.vname ext_vals;
				SkipChildren
		| _ -> SkipChildren
end

class externCondVisitor(f: file) = object(self)
	inherit collectBranchVisitor as super

	val vlist =
		let vis = new collectExternFuncNames in
		let _ = visitCilFileSameGlobals (vis :> cilVisitor) f in
		let func_queue = vis#get in queue_to_list func_queue;

	method extract_features (feat: feature) : feature =
		{ feat with has_extern_in_cond = super#get |> queue_to_list }

	method vstmt (s: stmt) =
		match s.skind with
		| If (exp, _, _, _) ->  
				List.iter (fun succ_node ->
					match succ_node.skind with
					| Instr i ->
						if((List.length i) !=0) then(
							begin
							match exp, get_branch_info (List.hd i) with
							| BinOp (_, exp1, exp2, _), Some(bid, _) ->
								begin 
								match exp1, exp2 with
								| Lval (Var vi, _), _  when (List.mem vi.vname vlist) ->
								  super#push bid
								| _, Lval (Var vi, _) when (List.mem vi.vname vlist) ->
								  super#push bid
								| _, _ -> ()
								end
							| _, _ -> ()
							end)
					| _ -> ()
				) s.succs;
				DoChildren
		| _ -> DoChildren

    method vinst (i: instr) = SkipChildren
	method describe : unit =
		prerr_endline "externInCondVisitor"
end
class intCondVisitor = object(self)
	inherit collectBranchVisitor as super

	method extract_features (feat: feature) : feature =
		{ feat with has_int_in_cond = super#get |> queue_to_list }

	method vstmt (s: stmt) =
		match s.skind with
		| If(exp, _, _, _) -> 
		  List.iter (fun succ_node  ->
			     begin
			     match succ_node.skind with
			     | Instr i  -> 
			        if((List.length i) !=0)	then (	
				    	begin 
		            	match exp, get_branch_info (List.hd i) with
	                    | Cil.BinOp(_, exp1, exp2, _), Some(bid, _) ->
			          	  	begin
			          		match exp1, exp2 with
			          		| Cil.Const(CInt64 (_, _, _)), _ 
							| _, Cil.Const(CInt64 (_, _, _)) -> super#push bid
			          		| _, _ -> ()
	   		          		end
                        | Cil.Const(CInt64 (_, _, _)), Some(bid, _) -> super#push bid
						| _, _ -> ()
						end)
			     | _ -> () 
                 end) s.succs;  DoChildren   
		| _ -> DoChildren
	
        method vinst (i: instr) = SkipChildren
		method describe : unit =
		prerr_endline "constantInCondVisitor"
end

class constantStringVisitor = object(self)
	inherit collectBranchVisitor as super

	method extract_features (feat: feature) : feature =
		{ feat with has_constant_strings_in_cond = super#get |> queue_to_list }

	method vstmt (s: stmt) =
		match s.skind with
		| If(exp, _, _, _) -> 
		  List.iter(fun succ_node  ->
			     begin
			     match succ_node.skind with
			     | Instr i  -> 
			     	if((List.length i) !=0) then(	
				   		begin 
		                match exp, get_branch_info (List.hd i) with
	                    | Cil.BinOp(_, exp1, exp2, _), Some(bid, _) -> 
			                begin
			          		match exp1, exp2 with
			          		| Cil.Const(CInt64 (a1, _, _)), _ 
							  when (snd (truncateInteger64 IInt a1)=false) && (i64_to_int a1)<128 && (i64_to_int a1)>=0 -> 
			          		  super#push bid
							| _, Cil.Const(CInt64 (a1, _, _)) 
							  when (snd (truncateInteger64 IInt a1)=false) && (i64_to_int a1)<128 && (i64_to_int a1)>=0 -> 
			          		  super#push bid
			          		| _, _ -> ()
	   		          		end
                       	| Cil.Const(CInt64 (_, _, _)), Some(bid, _) -> 
					   		super#push bid
						| _, _ -> ()
						end)
			     | _ -> () 
                 end) s.succs;  DoChildren   
		| _ -> DoChildren
	
        method vinst (i: instr) = SkipChildren
		method describe : unit =
			prerr_endline "constantInCondVisitor"
end

class pointerCondVisitor = object(self)
	inherit collectBranchVisitor as super

	method extract_features (feat: feature) : feature =
		{ feat with has_pointer_in_cond = super#get |> queue_to_list }

	method vstmt (s: stmt) =
		match s.skind with
		| If(exp, _, _, _) -> 
		  	List.iter(fun succ_node ->
				begin
				match succ_node.skind with
			    | Instr i  -> 
			      if((List.length i) !=0) then(	
					begin 
		            match exp,get_branch_info (List.hd i) with
	                | Cil.BinOp(_, e1, e2, _), Some(bid, _) ->
			        	begin
			          	match e1, e2 with
			          	| CastE(_ ,Lval(Mem _, _)), _  
			          	| _, CastE(_, Lval(Mem _, _))  
			          	| Lval(Mem _, _), _  
			          	| _, Lval(Mem _, _)  -> super#push bid 
				  	  	| _, _ -> ()
	   		        	end
	        		| Cil.Lval(Mem _, _) , Some(bid, _) -> super#push bid
					| _, _ -> ()  
					end)
		       	| _ -> () 
			    end) s.succs;  DoChildren   
		| _ -> DoChildren
     
	method vinst (i: instr) = SkipChildren
	method describe : unit =
		prerr_endline "pointerInCondVisitor"
end

class localVariableCondVisitor = object(self)
	inherit collectBranchVisitor as super

	method extract_features (feat: feature) : feature =
		{ feat with has_local_variable_in_cond = super#get |> queue_to_list }

	method vstmt (s: stmt) =
		match s.skind with
		| If(exp, _, _, _) -> 
		  	List.iter(fun succ_node  ->
				begin
			    match succ_node.skind with
			    | Instr i  -> 
			      if((List.length i) !=0) then(
					begin 
		            match exp, get_branch_info (List.hd i) with
	               	| Cil.BinOp(_, e1, e2, _), Some(bid,_) ->
			          begin
			          match e1, e2 with
			          | CastE(_ ,Lval(Var i, _)), _   when i.vglob = false -> super#push bid 
			          | _, CastE(_, Lval(Var i, _))  when i.vglob = false -> super#push bid
			          | Lval(Var i, _), _  when i.vglob = false -> super#push bid 	
					  | _, Lval(Var i, _) when i.vglob = false -> super#push bid 
				  	  | _, _ -> ()
	   		          end
	             	| Cil.Lval(Var i, _) , Some(bid, _) when(i.vglob = false) -> super#push bid 
					| _, _ -> ()  
					end)
			    | _ -> () 
                end) s.succs;  DoChildren   
		| _ -> DoChildren

  	method vinst (i: instr) = SkipChildren
	method describe : unit =
		prerr_endline "LocalVarInCondVisitor"
end

class loopBodyVisitor = object(self)
	inherit collectBranchVisitor as super

	method extract_features (feat: feature) : feature =
		{ feat with is_in_loopbody = super#get |> queue_to_list }
       
	method vstmt (s: stmt) =
		match s.skind with
		| Loop (b, _, _, _) -> let vis = new collectAllBranchVisitor in
		                       let _ = visitCilBlock (vis :> cilVisitor) b in
				       		   Queue.iter(fun bid -> Queue.push bid super#get) vis#get;
				       		   DoChildren
	    | _-> DoChildren			        
	 
	method vinst (i: instr) = SkipChildren
    method describe : unit =
    	prerr_endline "InWhileLoop"
end

class caseStatementVisitor (pred: bool) = object(self)
	inherit collectBranchVisitor as super
		
	method extract_features (feat: feature) : feature =
		match pred with
		| true -> { feat with is_Tbranch_casestatement = super#get |> queue_to_list }
		| false -> { feat with is_Fbranch_casestatement = super#get |> queue_to_list }
	
	method vstmt (s: stmt) =
		match s.skind with
	    | If (exp, bt, bf, _) ->
		  	List.iter (fun stm -> 
		  	begin	
			match stm.skind, (List.hd s.succs).skind with
			| Goto(refs,_), Instr i -> 
				if((List.length i) !=0) then(
					List.iter(fun label -> 
	                	begin 
		                match label, get_branch_info (List.hd i) with
	                    | Label(s,_,_), Some(bid, _)  
						  when (String.contains s 'c') && (String.contains s 'a') && (String.contains s 's') && (String.contains s 'e')  ->
						  if(pred = true) then super#push bid else super#push (bid+1)
						| _, _-> ()  
						end) (!refs).labels)
            | _, _ -> ()	
			end) bt.bstmts; DoChildren	
		| _ -> DoChildren

	method vinst (i: instr) = SkipChildren
	method describe : unit =
		prerr_endline "is True_Switch Statement"
end
	                         
let collect_features (f: Cil.file) : feature =
	let visitors = [
		(new mainBranchVisitor);	
		(new loopCondVisitor true);
		(new loopCondVisitor false);
		(new nestedBranchVisitor);
		(new externCondVisitor f);
		(new intCondVisitor);
		(new constantStringVisitor);
		(new pointerCondVisitor);
		(new localVariableCondVisitor);
		(new loopBodyVisitor);
		(new caseStatementVisitor true);
		(new caseStatementVisitor false);
		] in
	List.fold_left (fun feat vis ->
		visitCilFileSameGlobals (vis :> cilVisitor) f;
		(* vis#print; *)
		vis#extract_features feat
	) empty_feature visitors

let fvector_of : bid -> feature -> string
= fun id f ->
  let mem = List.mem in
  let str = ""
  |> (if mem id f.is_Fbranch_casestatement then (^) "1 " else (^) "0 ") 
  |> (if mem id f.is_Tbranch_casestatement then (^) "1 " else (^) "0 ") 
  |> (if mem id f.is_in_loopbody then (^) "1 " else (^) "0 ") 
  |> (if mem id f.has_local_variable_in_cond then (^) "1 " else (^) "0 ") 
  |> (if mem id f.has_pointer_in_cond then (^) "1 " else (^) "0 ") 
  |> (if mem id f.has_constant_strings_in_cond then (^) "1 " else (^) "0 ")
  |> (if mem id f.has_int_in_cond then (^) "1 " else (^) "0 ")
  |> (if mem id f.has_extern_in_cond then (^) "1 " else (^) "0 ")
  |> (if mem id f.is_nested then (^) "0 " else (^) "1 ")
  |> (if mem id f.is_Fbranch_loop then (^) "1 " else (^) "0 ")
  |> (if mem id f.is_Tbranch_loop then (^) "1 " else (^) "0 ")
  |> (if mem id f.is_in_main_func then (^) "1 " else (^) "0 ") in
  str 

let write_feature : Cil.file -> string -> feature -> unit
= fun f filename feature ->
  let vis = new collectAllBranchVisitor in
  let _ = visitCilFileSameGlobals (vis :> cilVisitor) f in
	let bidlist = vis#get |> queue_to_list in

  let output = open_out_gen [Open_creat; Open_append] 0o664 filename in
  let fvectorStr = List.fold_left (fun l i -> ((string_of_int i) ^ " : " ^ (fvector_of i feature))::l) [] bidlist in      
  let _ = List.iter (fun fvstr -> fprintf output "%s\n" fvstr) fvectorStr in
  let _ = flush output in
  close_out output

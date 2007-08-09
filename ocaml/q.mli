(*
 * Copyright (c) 2007 Fermin Reig (fermin@xrnd.com)
 *
 * q.mli
 *)

open Bigarray

type char_bigarray =    (char, int8_unsigned_elt, c_layout) Array1.t
type uint8_bigarray =   (int, int8_unsigned_elt, c_layout) Array1.t
type uint16_bigarray =  (int, int16_unsigned_elt, c_layout) Array1.t
type int32_bigarray =   (int32, int32_elt, c_layout) Array1.t
type int64_bigarray =   (int64, int64_elt, c_layout) Array1.t
type float32_bigarray = (float, float32_elt, c_layout) Array1.t
type float64_bigarray = (float, float64_elt, c_layout) Array1.t

(* Attributes of composite Q values *)

type attrib = 
  | A_none
  | A_s
  | A_u
  | A_p
  | A_g

(* The type of Q values *)
(* Note: there are no q enumerations. In the q-rpc protocol they are symbol vectors *)
(* Note: lambdas, operators, partial applications (types 100, 102 and 104) are not supported in this version*)

type  q_val = 
  (* scalars *)
  | Q_bool of bool
  | Q_byte of int
  | Q_short of int
  | Q_int32 of int32
  | Q_int64 of int64
  | Q_float32 of float
  | Q_float64 of float
  | Q_char of char
  | Q_symbol of string
  | Q_month of int32
  | Q_date of int32
  | Q_datetime of float
  | Q_minute of int32
  | Q_second of int32
  | Q_time of int32
  (* vectors of scalars *)
  | Q_v_bool of uint8_bigarray * attrib
  | Q_v_byte of uint8_bigarray * attrib
  | Q_v_short of uint16_bigarray * attrib
  | Q_v_int32 of int32_bigarray * attrib
  | Q_v_int64 of int64_bigarray * attrib
  | Q_v_float32 of float32_bigarray * attrib
  | Q_v_float64 of float64_bigarray * attrib
  | Q_v_char of char_bigarray * attrib
  (* Note: string array could also be a char_bigarray array, 
     or 2-dim char_bigarray *)
  | Q_v_symbol of string array * attrib
  | Q_v_month of int32_bigarray * attrib
  | Q_v_date of int32_bigarray * attrib
  | Q_v_datetime of float64_bigarray * attrib
  | Q_v_minute of int32_bigarray * attrib
  | Q_v_second of int32_bigarray * attrib
  | Q_v_time of int32_bigarray * attrib
  (* mixed lists *)
  | Q_mixed_list of q_val array
  (* tables and dictionaries *)
  (* TODO: have a special case for keyed tables? *)
  | Q_table of q_table
  | Q_dict of q_dict
  (* result of Q functions that return void. In q, (::) of type 101 *)
  | Q_unit


(* COULDDO: make the following two types private, as q_vals inside them
   cannot be arbitrary and must satisfy invariants *)

and q_dict = { keys: q_val; vals: q_val; attrib_d: attrib }

and q_table = { colnames: q_val; (* Always a Q_v_symbol *)
                cols: q_val;
		attrib_t: attrib }


type q_conn (* abstract *)

exception Q_connect of string

val q_connect : string -> int -> q_conn

(* COULDDO: export funs to check invariants of dicts and tables, as well as
   checked/unchecked rpcs *)

external q_eval_async : q_conn -> string -> unit = "q_eval_async"

external q_eval : q_conn -> string -> q_val = "q_eval"

external q_rpc_async : q_conn -> string -> q_val -> unit = "q_rpc_async"

external q_rpc : q_conn -> string -> q_val -> q_val = "q_rpc"


(* Note: sending a mixed list and receiving it back via the q identity 
   function is not always idempotent. For instance, if we construct a mixed 
   list in caml containing 0b and 1b and send it to a kdb instance, kdb turns
   it into 01b (a bool vector). 
*)


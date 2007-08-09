/*
 * Copyright (c) 2007 Fermin Reig (fermin@xrnd.com)
 *
 * q_interface.c
 */

// Uncomment next line to disable assertions
// #define NDEBUG

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/custom.h>
#include <caml/fail.h>
#include <caml/bigarray.h>
#include "q_interface.h"

// forward declarations

static value q_to_caml(const K q_val);
static K caml_to_q(const value v);


///////////////////////////////////////////////
// Functions to convert K values to Caml values
///////////////////////////////////////////////


static value mk_caml_value(const int tag, value v) {
  CAMLparam1(v);
  CAMLlocal1(result);

  result = caml_alloc(1, tag);
  Store_field(result, 0, v);
  CAMLreturn(result);
}

static value mk_caml_value_two(const int tag, value v, value attrib) {
  CAMLparam2(v, attrib);
  CAMLlocal1(result);

  assert(Is_block(v));
  assert(Is_long(attrib));

  result = caml_alloc(2, tag);
  Store_field(result, 0, v);
  Store_field(result, 1, attrib);
  CAMLreturn(result);
}


static value mk_caml_dict(const K q_val) {
  CAMLparam0 ();
  CAMLlocal1 (result);

  result = caml_alloc(3, 0);
  K keys = kK(q_val)[0];
  K values = kK(q_val)[1];
  Store_field(result, 0, q_to_caml(keys));
  Store_field(result, 1, q_to_caml(values));
  Store_field(result, 2, Val_int(q_val->u)); // Atribute
  CAMLreturn (mk_caml_value(tag_dict, result));
  }

static value mk_caml_table(const K q_val) {
  CAMLparam0 ();
  CAMLlocal1 (tbl);

  tbl = caml_alloc(3, 0);
  Store_field(tbl, 0, q_to_caml(kK(q_val->k)[0]));
  Store_field(tbl, 1, q_to_caml(kK(q_val->k)[1]));
  Store_field(tbl, 2, Val_int(q_val->u)); // Attribute
  CAMLreturn (mk_caml_value(tag_table, tbl));
}


static value mk_caml_byte_array(const int caml_tag, const K q_val) {
  CAMLparam0 ();
  CAMLlocal2 (attrib, arr);

  long dims[1];
  dims[0] = q_val->n;
  attrib = Val_int(q_val->u);
  arr = alloc_bigarray(BIGARRAY_UINT8 | BIGARRAY_C_LAYOUT, 1, kG(q_val), dims);
  CAMLreturn (mk_caml_value_two(caml_tag, arr, attrib));
}

static value mk_caml_scalar_array(const int caml_tag, const int arr_ty, void * data, const K q_val) {
  CAMLparam0 ();
  CAMLlocal2 (attrib, arr);

  long dims[1];
  dims[0] = q_val->n;
  attrib = Val_int(q_val->u);
  arr = alloc_bigarray(arr_ty | BIGARRAY_C_LAYOUT, 1, data, dims);
  CAMLreturn (mk_caml_value_two(caml_tag, arr, attrib));
}


// See also mk_caml_string_array_helper
static value mk_caml_array(const K q_val)
{
  // Note: this is largely the same as the funcion caml_alloc_array in the caml
  // RTS (alloc.c)
  CAMLparam0 ();
  CAMLlocal2 (v, result);

  const unsigned long size = q_val->n;
  // The current implementation of Ocaml (June 2007) supports arrays
  // containing up to 2^22 - 1 (4194303) elements on 32-bit platforms and
  // 2^54 - 1 on 64-bit platforms.

  if (0 == size) {
    CAMLreturn (Atom(0));
  } else {
    result = caml_alloc(size, 0);
    K *q_elems = kK(q_val);
    unsigned long i;
    for (i = 0; i < size; i++) {
      /* The two statements below must be separate because of evaluation
         order (don't take the address &Field(result, i) before
         calling q_to_caml, which may cause a GC and move result). */
      v = q_to_caml(q_elems[i]);
      caml_modify(&Field(result, i), v);
    }
    CAMLreturn (result);
  }
}

// See also mk_caml_array
static value mk_caml_string_array_helper(const K q_val) {
  CAMLparam0 ();
  CAMLlocal2 (v, result);

  assert(q_val->t > 0);

  const unsigned long size = q_val->n;
  if(0 == size) {
    CAMLreturn(Atom(0));
  } else {
    result = caml_alloc(size, 0);
    unsigned char **q_arr = kS(q_val);
    unsigned long i;
    for (i = 0; i < size ; i++) {
      // The two statements below must be separate because of evaluation
      // order (don't take the address &Field(result, i) before
      // calling caml_copy_string, which may cause a GC and move result).
      v = caml_copy_string(q_arr[i]);
      caml_modify(&Field(result, i), v);
    }
    CAMLreturn(result);
  }
}

static value mk_caml_string_array(const K q_val) {
  CAMLparam0 ();
  CAMLlocal2 (attrib, arr);

  attrib = Val_int(q_val->u);
  arr = mk_caml_string_array_helper(q_val);
  CAMLreturn (mk_caml_value_two(tag_v_symbol, arr, attrib));
}


static int tag_for_scalar(const int ty) {
  switch(ty){
  case (t_int32):  return tag_int32;
  case (t_month):  return tag_month;
  case (t_date):   return tag_date;
  case (t_minute): return tag_minute;
  case (t_second): return tag_second;
  case (t_time):   return tag_time;
  default: {
    caml_failwith("tag_for_scalar: impossible tag\n");
  }
  }
}

static int tag_for_vector(const int ty) {
  switch(ty){
  case (-t_bool):     return tag_v_bool;
  case (-t_byte):     return tag_v_byte;
  case (-t_int16):    return tag_v_int16;
  case (-t_int32):    return tag_v_int32;
  case (-t_int64):    return tag_v_int64;
  case (-t_float32):  return tag_v_float32;
  case (-t_float64):  return tag_v_float64;
  case (-t_char):     return tag_v_char;
  case (-t_month):    return tag_v_month;
  case (-t_date):     return tag_v_date;
  case (-t_datetime): return tag_v_datetime;
  case (-t_minute):   return tag_v_minute;
  case (-t_second):   return tag_v_second;
  case (-t_time):     return tag_v_time;
  default: {
    caml_failwith("tag_for_vector: impossible tag\n");
  }
  }
}


// Convert K->caml
static value q_to_caml(const K q_val) {
  const H q_type = q_val->t; 
  switch(q_type) {

  // Scalars

  case t_bool: {
    return (mk_caml_value(tag_bool, Val_bool(q_val->g)));
  }
  case t_byte: {
    return (mk_caml_value(tag_byte, Val_int(q_val->g)));
  }
  case t_int16: {
    return (mk_caml_value(tag_int16, Val_int(q_val->h)));
  }
  case t_int32: 
  case t_month: 
  case t_date: 
  case t_minute: 
  case t_second: 
  case t_time: 
  {
    return (mk_caml_value(tag_for_scalar(q_type), caml_copy_int32(q_val->i)));
  }

  case t_int64: {
    return (mk_caml_value(tag_int64, caml_copy_int64(q_val->j)));
  }
  case t_float32: {
    return (mk_caml_value(tag_float32, caml_copy_double(q_val->e)));
  }
  case t_float64: {
    return (mk_caml_value(tag_float64, caml_copy_double(q_val->f)));
  }
  case t_char: {
    return(mk_caml_value(tag_char,  Val_int(q_val->g)));
  }
  case t_symbol: {
    return (mk_caml_value(tag_symbol, caml_copy_string(q_val->s)));
  }
  case t_datetime: {
    return (mk_caml_value(tag_datetime, caml_copy_double(q_val->f)));
  }

  // Vectors of scalars

  case (-t_bool): 
  case (-t_byte): 
  case (-t_char): {
    return (mk_caml_byte_array(tag_for_vector(q_type), q_val));
  }
  case (-t_int16): {
    return mk_caml_scalar_array(tag_for_vector(q_type), BIGARRAY_UINT16, kH(q_val), q_val); 
  }
  case (-t_int32):
  case (-t_month): 
  case (-t_date):
  case (-t_minute):
  case (-t_second):
  case (-t_time): 
  {
    return mk_caml_scalar_array(tag_for_vector(q_type), BIGARRAY_INT32, kI(q_val), q_val); 
  }
  case (-t_int64):  {
    return mk_caml_scalar_array(tag_for_vector(q_type), BIGARRAY_INT64, kJ(q_val), q_val); 
  }
  case (-t_float32): {
    return mk_caml_scalar_array(tag_for_vector(q_type), BIGARRAY_FLOAT32, kE(q_val), q_val); 
  }
  case (-t_float64): 
  case (-t_datetime): {
    return mk_caml_scalar_array(tag_for_vector(q_type), BIGARRAY_FLOAT64, kF(q_val), q_val); 
  }
  case (-t_symbol): {
    return mk_caml_string_array(q_val);
  }

  // Mixed lists

  case t_mixed_list: {
    return (mk_caml_value(tag_mixed_list, mk_caml_array(q_val)));;
  }

  // Tables

  case t_table:{
    return (mk_caml_table(q_val));
  }

  // Dictionaries

  case t_dict: {
    return (mk_caml_dict(q_val));
  }

  case t_unit: {
    return (Val_int(tag_unit));
  }
  case t_lambda: {
    caml_failwith("Not supported: lambda (type 100)");
  }
  case t_operator: {
    caml_failwith("Not supported: q operator (type 102)");
  }
  case t_partial_app: {
    caml_failwith("Not supported: partial application (type 104)");
  }
  case t_error: {
      caml_failwith(q_val->s);
  }
  default: {
    fprintf(stderr, "internal error: q_to_caml impossible q type %i\n", q_type);
    caml_failwith("internal error: q_to_caml impossible q type");
  }
  }
}

///////////////////////////////////////////////
// Functions to convert Caml values to K values
///////////////////////////////////////////////

// Q's C interface does not provide functions to build values of type
// month, date, minute, second. The following will do.

static K mk_int_scalar(const int ty, const int i) 
{
  K result = ki(i);
  result->t = ty;
  return result;
}


static int tag_to_type(const int tag) {
  switch(tag){
  case tag_month:    return (t_month);
  case tag_date:     return (t_date);
  case tag_minute:   return (t_minute);
  case tag_second:   return (t_second);
  default: {
    fprintf(stderr, "tag_to_type: impossible tag %i\n", tag);
    caml_failwith("tag_to_type: impossible tag");
  }
  }
}

static int tag_to_v_type(const int tag) {
  switch(tag){
  case tag_v_bool:     return (-t_bool);
  case tag_v_byte:     return (-t_byte);
  case tag_v_int32:    return (-t_int32);
  case tag_v_float32:  return (-t_float32);
  case tag_v_float64:  return (-t_float64);
  case tag_v_char:     return (-t_char);
  case tag_v_month:    return (-t_month);
  case tag_v_date:     return (-t_date);
  case tag_v_datetime: return (-t_datetime);
  case tag_v_minute:   return (-t_minute);
  case tag_v_second:   return (-t_second);
  case tag_v_time:     return (-t_time);
  default: {
    fprintf(stderr, "tag_to_v_type: impossible tag %i\n", tag);
    caml_failwith("tag_to_v_type: impossible tag");
  }
  }
}


static K mk_scalar_vector(const unsigned elem_size, const int ty, const value v) {
  assert (Is_block(v));

  const value arr = Field(v,0);

  assert (Is_block(arr));
  assert (1 == Bigarray_val(arr)->num_dims);

  // TODO: what if the size is more than what fits in an int32?
  // Can you have vectors longer than 2^32 elems in kdb?
  const int size = Bigarray_val(arr)->dim[0];
  K list = ktn(ty, size);
  list->u = (short)Int_val(Field(v,1)); // Attribute
  memcpy(list->G0, Data_bigarray_val(arr), size * elem_size);
  return list;
}


static inline K mk_typed_byte_vector(const int ty, const value v) {
  return(mk_scalar_vector(sizeof(char), ty, v));
}

static inline K mk_int16_vector(const value v) {
  return(mk_scalar_vector(sizeof(short), KH, v));
}

static inline K mk_typed_int32_vector(const int ty, const value v) {
  return(mk_scalar_vector(sizeof(int), ty, v));
}

static inline K mk_int64_vector(const value v) {
  return(mk_scalar_vector(sizeof(int64_t), KJ, v));
}

static inline K mk_float32_vector(const int ty, const value v) {
  return(mk_scalar_vector(sizeof(float), KE, v));
}

static inline K mk_float64_vector(const int ty, const value v) {
  return(mk_scalar_vector(sizeof(double), ty, v));
}

static K mk_char_vector(const int ty, const value v) {
  assert (Is_block(v));

  const value arr = Field(v,0);

  assert (Is_block(arr));
  assert (1 == Bigarray_val(arr)->num_dims);

  K vec = kp((unsigned char *)Data_bigarray_val(arr));
  vec->u = (short)Int_val(Field(v,1)); // Attribute
  return vec;
}


static K mk_symbol_vector(const value v) {
  assert (Is_block(v));

  const value arr = Field(v,0);
  assert (Is_block(arr));
  // TODO: what if the size is more than what fits in an int32?
  // Can you have vectors longer than 2^32 elems in kdb?
  const int count = Wosize_val(arr);
  K list = ktn(KS, 0);
  unsigned i;
  for(i=0; i<count; i++) {
    unsigned char* elem = ss((unsigned char *)String_val(Field(arr,i)));
    js(&list, elem);
  }
  // Note: attribute must be set after calling js
  list->u = (short)Int_val(Field(v,1)); // Attribute
  return list;
}


static K mk_mixed_list(const value v) {
  assert (Is_block(v));

  // TODO: what if the size is more than what fits in an int32?
  // Can you have vectors longer than 2^32 elems in kdb?
  const int count = Wosize_val(v);
  K list = knk(0);
  unsigned i;
  for(i=0; i<count; i++) {
    jk(&list, caml_to_q(Field(v, i)));
  }
  return list;
}


// Convert caml value of type q_val to K value
static K caml_to_q(const value val)
{
  if(!Is_block(val)) {
    // Q_unit
    assert(tag_unit == Long_val(val));
    // Note: kdb's C interface does not export a function to create units
    K result = kb(0);
    result->t = 101;
    return result;
  } else {
    value v = Field(val, 0);
    const int tag = Tag_val(val);
    switch(tag) {

    // Scalars

    case tag_bool: {
      return kb(Bool_val(v));
    }
    case tag_byte: {
      return kg(Int_val(v));
    }
    case tag_int16: {
      return kh(Int_val(v));
    }
    case tag_int32: {
      return ki(Int32_val(v));
    }
    case tag_int64: {
      return kj(Int64_val(v));
    }
    case tag_float32: {
      return ke(Double_val(v));
    }
    case tag_float64: {
      return kf(Double_val(v));
    }
    case tag_char: {
      return kc(Int_val(v));
    }
    case tag_symbol: {
      // Note: intern the string to create the symbol
      return ks(ss((unsigned char *)String_val(v)));
    }
    case tag_month: 
    case tag_date:
    case tag_minute:
    case tag_second:
      {
	return mk_int_scalar(tag_to_type(tag), Int32_val(v));
      }
    case tag_datetime: {
      return kz(Double_val(v));
    }
    case tag_time: {
      return kt(Int32_val(v));
    }

    // Vectors of scalars

    case tag_v_bool:
    case tag_v_byte: {
      return mk_typed_byte_vector(tag_to_v_type(tag), val);
    }
    case tag_v_int16: {
      return mk_int16_vector(val);
    }
    case tag_v_int32:  
    case tag_v_month:  
    case tag_v_date:   
    case tag_v_minute: 
    case tag_v_second: 
    case tag_v_time: {
      return mk_typed_int32_vector(tag_to_v_type(tag), val);
    }  
    case tag_v_int64: {
      return mk_int64_vector(val);
    }
    case tag_v_char: {
      return mk_char_vector(tag_to_v_type(tag), val);
      //K arr = kp((unsigned char *)Data_bigarray_val(v));
      //return kp((unsigned char *)Data_bigarray_val(v));
    }
    case tag_v_float32: { 
      return mk_float32_vector(tag_to_v_type(tag), val);
    }
    case tag_v_float64: 
    case tag_v_datetime: {
      return mk_float64_vector(tag_to_v_type(tag), val);
    }
    case tag_v_symbol: {
      return mk_symbol_vector(val);
    }

    // Mixed lists

    case tag_mixed_list: {
      return mk_mixed_list(v);
    }

    // Tables

    case tag_table: {
      K colnames = caml_to_q(Field(v, 0));
      K cols     = caml_to_q(Field(v, 1));
      K dict = xD(colnames, cols);
      dict->u = (short)Int_val(Field(v, 2)); // Atribute
      return(xT(dict));
    }
    case tag_dict: {
      K keys   = caml_to_q(Field(v, 0));
      K values = caml_to_q(Field(v, 1));
      K dict = xD(keys, values);
      dict->u = (short)Int_val(Field(v, 2)); // Atribute
      return(dict);
    }

    default: {
      fprintf(stderr, "caml_to_q: invalid tag %i\n",  tag);
      caml_failwith("caml_to_q impossible caml tag");
    }
    }
  }
}

///////////////////////////////////////////////////
// Exported Caml functions to talk to kdb instances
///////////////////////////////////////////////////

CAMLprim value q_connect(value host, value port)
{
  CAMLparam2(host, port);
  const int q_instance = khp(String_val(host), Int_val(port));
  CAMLreturn(caml_copy_int32(q_instance));
}

CAMLprim value q_eval_async(value q_conn, value str)
{
  CAMLparam2(q_conn, str);

  assert(Is_block(str));

  k(-Int32_val(q_conn), String_val(str), (K)0);
  CAMLreturn(Val_unit);
}

CAMLprim value q_eval(value q_conn, value str)
{
  CAMLparam2(q_conn, str);
  CAMLlocal1(result);

  assert(Is_block(str));

  K reply = k(Int32_val(q_conn), String_val(str), (K)0);
  result = q_to_caml(reply);
  // Free the memory for 'reply'
  r0(reply);
  CAMLreturn(result);
}


CAMLprim value q_rpc_async(value q_conn, value str, value val)
{
  CAMLparam3(q_conn, str, val);

  assert(Is_block(str));

  k(-Int32_val(q_conn), String_val(str), caml_to_q(val), (K)0);
  CAMLreturn(Val_unit);
}

CAMLprim value q_rpc(value q_conn, value str, value val)
{
  CAMLparam3(q_conn, str, val);
  CAMLlocal1(result);

  assert(Is_block(str));

  K reply = k(Int32_val(q_conn), String_val(str), caml_to_q(val), (K)0);
  result = q_to_caml(reply);
  // Free the memory for 'reply'
  r0(reply);
  CAMLreturn(result);
}


/**

Q values in caml (using the array interface)

trade

time         sym price size
---------------------------
09:30:00.000 a   10.75 100
09:30:00.000 a   10.75 100
09:30:00.000 a   10.75 100


  Q.Q_table
   {Q.colnames = Q.Q_v_symbol [|"time"; "sym"; "price"; "size"|];
    Q.cols =
     Q.Q_mixed_list
      [|Q.Q_v_time [|34200000l; 34200000l; 34200000l|];
        Q.Q_v_symbol [|"a"; "a"; "a"|];
        Q.Q_v_float64 [|10.75; 10.75; 10.75|];
        Q.Q_v_int32 [|100l; 100l; 100l|]|]}

flip trade

  Q.Q_dict
   {Q.keys = Q.Q_v_symbol [|"time"; "sym"; "price"; "size"|];
    Q.vals =
     Q.Q_mixed_list
      [|Q.Q_v_time [|34200000l; 34200000l; 34200000l|];
        Q.Q_v_symbol [|"a"; "a"; "a"|];
        Q.Q_v_float64 [|10.75; 10.75; 10.75|];
        Q.Q_v_int32 [|100l; 100l; 100l|]|]}

sym| time                                   price             size
---| --------------------------------------------------------------------
a  | 09:30:00.000 09:30:00.000 09:30:00.000 10.75 10.75 10.75 100 100 100

val v : Q.q_val =
  Q.Q_dict
   {Q.keys =
     Q.Q_table
      {Q.colnames = Q.Q_v_symbol [|"sym"|];
       Q.cols = Q.Q_mixed_list [|Q.Q_v_symbol [|"a"|]|]};
    Q.vals =
     Q.Q_table
      {Q.colnames = Q.Q_v_symbol [|"time"; "price"; "size"|];
       Q.cols =
        Q.Q_mixed_list
         [|Q.Q_mixed_list [|Q.Q_v_time [|34200000l; 34200000l; 34200000l|]|];
           Q.Q_mixed_list [|Q.Q_v_float64 [|10.75; 10.75; 10.75|]|];
           Q.Q_mixed_list [|Q.Q_v_int32 [|100l; 100l; 100l|]|]|]}}

As unkeyed table

val v : Q.q_val =
  Q.Q_table
   {Q.colnames = Q.Q_v_symbol [|"sym"; "time"; "price"; "size"|];
    Q.cols =
     Q.Q_mixed_list
      [|Q.Q_v_symbol [|"a"|];
        Q.Q_mixed_list [|Q.Q_v_time [|34200000l; 34200000l; 34200000l|]|];
        Q.Q_mixed_list [|Q.Q_v_float64 [|10.75; 10.75; 10.75|]|];
        Q.Q_mixed_list [|Q.Q_v_int32 [|100l; 100l; 100l|]|]|]}

sym time        | price             size
----------------| -----------------------------
a   09:30:00.000| 10.75 10.75 10.75 100 100 100
b   09:45:00.000| 11 11f            50 50


  Q.Q_dict
   {Q.keys =
     Q.Q_table
      {Q.colnames = Q.Q_v_symbol [|"sym"; "time"|];
       Q.cols =
        Q.Q_mixed_list
         [|Q.Q_v_symbol [|"a"; "b"|]; 
	   Q.Q_v_time [|34200000l; 35100000l|]|]};
    Q.vals =
     Q.Q_table
      {Q.colnames = Q.Q_v_symbol [|"price"; "size"|];
       Q.cols =
        Q.Q_mixed_list
         [|Q.Q_mixed_list
            [|Q.Q_v_float64 [|10.75; 10.75; 10.75|];
              Q.Q_v_float64 [|11.; 11.|]|];
           Q.Q_mixed_list
            [|Q.Q_v_int32 [|100l; 100l; 100l|]; 
	      Q.Q_v_int32 [|50l; 50l|]|]|]}}

As unkeyed table

  Q.Q_table
   {Q.colnames = Q.Q_v_symbol [|"sym"; "time"; "price"; "size"|];
    Q.cols =
     Q.Q_mixed_list
      [|Q.Q_v_symbol [|"a"; "b"|]; 
        Q.Q_v_time [|34200000l; 35100000l|];
        Q.Q_mixed_list
         [|Q.Q_v_float64 [|10.75; 10.75; 10.75|];
           Q.Q_v_float64 [|11.; 11.|]|];
        Q.Q_mixed_list
         [|Q.Q_v_int32 [|100l; 100l; 100l|]; 
	   Q.Q_v_int32 [|50l; 50l|]|]|]}


**/

/**
Note: this version turns keyed tables into unkeyed ones 
static value mk_caml_dict_old(const K q_val) {
  CAMLparam0 ();
  CAMLlocal1 (dict);

  K keys = kK(q_val)[0];
  K values = kK(q_val)[1];
  //printf("keys %i\n", keys->t);
  //printf("vals %i\n", values->t);
  //printf("t_table %i\n", t_table);
  if ((t_table == keys->t) && (t_table == values->t)) {
    // return as unkeyed table (but then, is info lost?)
    CAMLreturn(q_to_caml(ktd(q_val)));
  } else {
    // return as dict
    dict = caml_alloc(3, 0);
    Store_field(dict, 0, q_to_caml(keys));
    Store_field(dict, 1, q_to_caml(values));
    Store_field(dict, 2, Val_int(q_val->u)); // Atribute
    CAMLreturn (mk_caml_value(tag_dict, dict));
  }
}
**/

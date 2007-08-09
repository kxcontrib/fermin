/*
 * Copyright (c) 2007 Fermin Reig (fermin@xrnd.com)
 *
 * q_interface.h
 */

#ifndef _Q_INTERFACE_H_
#define	_Q_INTERFACE_H_

#include "k.h"
#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>

enum q_types {
  // scalars
  t_bool        = -KB,
  t_byte        = -KG,
  t_int16       = -KH, /* short */
  t_int32       = -KI,
  t_int64       = -KJ,
  t_float32     = -KE, /* real */
  t_float64     = -KF, /* float */
  t_char        = -KC,
  t_symbol      = -KS,
  t_month       = -KM,
  t_date        = -KD,
  t_datetime    = -KZ,
  t_minute      = -KU,
  t_second      = -KV,
  t_time        = -KT,
  // vectors of scalars. (not represented explicitly: use the negated scalar)
  // mixed lists
  t_mixed_list  = 0,
  // tables and dictionaries
  t_table       = XT,
  t_dict        = XD,
  // misc
  t_unit        = 101,
  // lambda, operator, partial app: not yet implemented
  t_lambda      = 100,
  t_operator    = 102,
  t_partial_app = 104,
  t_error       = -128
};

enum caml_tags {
  // scalars 
  tag_bool,        
  tag_byte,        
  tag_int16,       
  tag_int32,       
  tag_int64,       
  tag_float32,     
  tag_float64,     
  tag_char,        
  tag_symbol,      
  tag_month,       
  tag_date,        
  tag_datetime,    
  tag_minute,      
  tag_second,      
  tag_time,
  // vectors of scalars
  tag_v_bool,
  tag_v_byte,
  tag_v_int16, 
  tag_v_int32,  
  tag_v_int64,  
  tag_v_float32, 
  tag_v_float64, 
  tag_v_char,
  tag_v_symbol,
  tag_v_month,
  tag_v_date,
  tag_v_datetime, 
  tag_v_minute, 
  tag_v_second,
  tag_v_time,
  // mixed lists
  tag_mixed_list,  
  // tables and dictionaries
  tag_table,       
  tag_dict,
  // result of Q functions that return void
  // Implementation note: caml constant constructors are numbered separately
  // from non-constant ones
  tag_unit = 0
};


#endif /* _Q_INTERFACE_H_ */

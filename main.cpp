#include <exception>
#include <iostream>
#include <sstream>
#include <stack>
#include <memory>
#include <vector>
#include <map>
#include <stdexcept>
#include <functional>
#include <cassert>
#include <csignal>

bool g_keep_running = true;

void sig_handler(int s) {
  if (s == SIGINT) {
    std::cout << "Got SIGINT. Shutdown gracefully\n";
    g_keep_running = false;
  }
}



typedef double symbol_number_type;

struct ConsCell;
struct LispType {
  enum Type {
    nil,
    symbol,
    variable,
    number,
    cons,
    function
  };

  Type type;
  
  symbol_number_type number_val;
  std::string string_val;
  std::shared_ptr<ConsCell> cons_val;
};

void print_cons_recursive(const LispType &cons, int depth, std::ostream &o);
void print_lisp_type(const LispType &sym, bool with_type, std::ostream &o);

std::ostream& operator << (std::ostream& o, const LispType& a)
{
  print_lisp_type(a, false, o);
  return o; 
}

template <typename T>
LispType make_number(T number)
{
  return {
    .type = LispType::Type::number,
    .number_val = static_cast<symbol_number_type>(number)
  };
}

LispType make_nil()
{
  return {
    .type = LispType::Type::nil
  };
}

void print_lisp_type(const LispType &sym, bool with_type, std::ostream& o = std::cout)
{
  switch (sym.type) {
  case LispType::Type::nil:
    o << "nil";
    break;
  case LispType::Type::number:
    o << (with_type ? "[n] " : "") << sym.number_val;
    break;
  case LispType::Type::variable:
    o << (with_type ? "[v] " : "") << sym.string_val;
    break;
  case LispType::Type::symbol:
    o << (with_type ? "[s] " : "") << "'" << sym.string_val;
    break;
  case LispType::Type::function:
    o << (with_type ? "[f] " : "") << sym.string_val;
    break;
  case LispType::Type::cons:
    if (with_type) {
      o << "[c] ";
    }
    print_cons_recursive(sym, 0, o);
    break;
  default:
    std::stringstream ss;
    ss << "cant print type: ";
    ss << sym.type;
    throw std::runtime_error(ss.str());
    break;
  }
}

void eval(const LispType& code, LispType &result);
void parse(std::string sexp, LispType &root);

struct ConsCell {
  LispType head;
  std::shared_ptr<ConsCell> tail;

  ConsCell(const LispType &_h)
    : head(_h) {}
  ConsCell(LispType &&_h)
    : head(std::move(_h)) {}
  ConsCell(const LispType &_h, const std::shared_ptr<ConsCell> &_t)
    : head(_h), tail(_t) {}
  ConsCell(LispType &&_h, const std::shared_ptr<ConsCell> &_t)
    : head(std::move(_h)), tail(_t) {}
  
  ConsCell(const ConsCell &other) = default;
  ConsCell(ConsCell &&other) = default;

  ConsCell& operator=(const ConsCell& other) = default;
  ConsCell& operator=(ConsCell&& other) = default;
};

void print_cons_recursive(const LispType &cons, int depth, std::ostream &o)
{
  if (depth == 0) {
    o << "(";
  }
  //  std::cout << "type: " << cons.type << "\n";
  if (cons.type == LispType::Type::cons) {
    print_lisp_type(cons.cons_val->head, false);
    if (cons.cons_val->tail != nullptr &&
        cons.cons_val->tail->head.type != LispType::Type::nil) {
      o << " ";
      print_cons_recursive(cons.cons_val->tail->head, depth + 1, o);
    }
  } else {
    print_lisp_type(cons, false, o);
  }
  if (depth == 0) {
    o << ")";
  }
}

void cons(const LispType &a, const LispType &b, LispType &result)
{
  auto new_cons = std::make_shared<ConsCell>(a, std::make_shared<ConsCell>(b));
  
  result = {
    .type = LispType::Type::cons,
    .cons_val = new_cons
  };
}

void car(const LispType &a, LispType &result)
{
  // std::cout << "CAR: ";
  // print_lisp_type(a);
  // std::cout << "\n";
  assert (a.type == LispType::Type::cons);
  result = a.cons_val->head;
}

void cdr(const LispType &a, LispType &result)
{  
  assert (a.type == LispType::Type::cons);

  if (a.cons_val->tail == nullptr) {
    result = make_nil();
    return;
  }
  
  std::shared_ptr<ConsCell> tail = a.cons_val->tail;

  if (tail->head.type == LispType::Type::cons) {
    result = {
      .type = LispType::Type::cons,
      .cons_val = tail->head.cons_val
    };
  } else {
    // std::cout << "cdr: ";
    // print_lisp_type(tail->head, true);
    // std::cout << "\n";
    result = tail->head;
  }
}

void make_list(const std::vector<LispType> &args, LispType &result)
{
  if (args.size() == 0) {
    result = make_nil();
    return;
  }
  if (args.size() == 1) {
    cons(args[0], make_nil(), result);
    return;
  }
  result = make_nil();
  for (auto it = args.crbegin(); it != args.crend(); ++it) {
    // std::cout << "BLA : ";
    // print_lisp_type(*it);
    // std::cout << "\n";
    cons(*it, result, result);
  }
}

void iter_cons(const LispType &cons, int target_idx, int idx, LispType &result)
{
  if (idx == target_idx) {
    if (cons.type == LispType::Type::cons) {
      car(cons, result);
    } else {
      result = cons;
    }
    return;
  }

  if (cons.type == LispType::Type::cons) {
    LispType tail;
    cdr(cons, tail);
    iter_cons(tail, target_idx, idx + 1, result);
  } else {
    result = make_nil();
  }
}

void nth(const LispType &idx_type, const LispType &cons_type, LispType &result)
{
  if (idx_type.type != LispType::Type::number) {
    throw std::runtime_error("nth arg0 must be number");
  }
  if (cons_type.type != LispType::Type::cons) {
    throw std::runtime_error("nth arg1 must be cons cell");
  }
  int index = static_cast<int>(idx_type.number_val);
  if (index < 0) {
    throw std::runtime_error("nth arg0 must be positive number");
  }

  iter_cons(cons_type, index, 0, result);
}

std::map<std::string, LispType> g_variables;
std::map<std::string, std::function<void(std::vector<LispType>&, LispType&)>> g_builtins;


void builtin_dump_variables(const std::vector<LispType> &args, LispType &result_sym)
{
  for (auto it = g_variables.begin(); it != g_variables.end(); ++it) {
    const LispType &type = it->second;
    std::cout << it->first << "\t\t\t\t";
    print_lisp_type(type, true);
    std::cout << "\n";
  }
  result_sym = { .type = LispType::Type::nil };
}

void builtin_set(const std::vector<LispType> &args, LispType &result_sym)
{
  if (args.size() != 2) {
    throw std::runtime_error("set needs 2 args");
  }
  if (args[0].type != LispType::Type::symbol) {
    throw std::runtime_error("set arg0 must be symbol");
  }
  const std::string &symbol_name = args[0].string_val;

  const LispType &val = args[1];

  switch (val.type) {
  case LispType::Type::number:
    g_variables[symbol_name] = {
      .type = LispType::Type::number,
      .number_val = val.number_val
    };
    break;
  case LispType::Type::symbol:
    g_variables[symbol_name] = {
      .type = LispType::Type::symbol,
      .string_val = val.string_val
    };
    break;
  case LispType::Type::nil:
    g_variables[symbol_name] = {
      .type = LispType::Type::nil
    };
    break;
  case LispType::Type::cons:
    g_variables[symbol_name] = {
      .type = LispType::Type::cons,
      .cons_val = val.cons_val
    };
    break;
  default:
    throw std::runtime_error("set not implemented for type");
  }

  result_sym = g_variables[symbol_name];
}


void builtin_cons(const std::vector<LispType> &args, LispType &result_sym)
{
  if (args.size() == 0 || args.size() > 2) {
    throw std::runtime_error("cons requires at most 2 args");
  }

  if (args.size() == 1) {
    cons(args[0], make_nil(), result_sym);
    return;
  }
  
  cons(args[0], args[1], result_sym);
}

void builtin_car(const std::vector<LispType> &args, LispType &result_sym)
{
  if (args.size() > 1) {
    throw std::runtime_error("car requires 0 or 1 arg");
  }
  if (args[0].type == LispType::Type::nil) {
    result_sym = make_nil();
    return;
  }
  else if (args[0].type != LispType::Type::cons) {
    throw std::runtime_error("car arg0 must be cons cell");
  }

  car(args[0], result_sym);
}

void builtin_cdr(const std::vector<LispType> &args, LispType &result_sym)
{
  if (args.size() != 1) {
    throw std::runtime_error("cdr requires 1 arg");
  }
  if (args[0].type != LispType::Type::cons) {
    throw std::runtime_error("cdr arg0 must be cons cell");
  }

  cdr(args[0], result_sym);
}

void builtin_nth(const std::vector<LispType> &args, LispType &result_sym)
{
  if (args.size() != 2) {
    throw std::runtime_error("nth requires 2 args");
  }

  nth(args[0], args[1], result_sym);
}

void builtin_list(const std::vector<LispType> &args, LispType &result_sym)
{
  make_list(args, result_sym);
}

void builtin_add(const std::vector<LispType> &args, LispType &result_sym)
{
  symbol_number_type result = 0.0;
  for (auto it = args.cbegin(); it != args.cend(); ++it) {
    const LispType &s = *it;
    if (s.type != LispType::Type::number) {
      throw std::runtime_error("symbol not a number");
    }
    result += s.number_val;
  }
  result_sym.type = LispType::Type::number;
  result_sym.number_val = result;
}

void builtin_mul(const std::vector<LispType> &args, LispType &result_sym)
{
  symbol_number_type result = 1.0;
  for (auto it = args.cbegin(); it != args.cend(); ++it) {
    const LispType &s = *it;
    if (s.type != LispType::Type::number) {
      throw std::runtime_error("symbol not a number");
    }
    result *= s.number_val;
  }
  result_sym.type = LispType::Type::number;
  result_sym.number_val = result;
}


void builtin_min(const std::vector<LispType> &args, LispType &result_sym)
{
  if (args.size() == 0 || args[0].type != LispType::Type::number) {
    throw std::runtime_error("invalid args");
  }
  symbol_number_type result = args[0].number_val;
  for (auto it = args.cbegin() + 1; it != args.cend(); ++it) {
    const LispType &s = *it;
    if (s.type != LispType::Type::number) {
      throw std::runtime_error("symbol not a number");
    }
    result -= s.number_val;
  }
  result_sym.type = LispType::Type::number;
  result_sym.number_val = result;
}

void builtin_div(const std::vector<LispType> &args, LispType &result_sym)
{
  if (args.size() == 0 || args[0].type != LispType::Type::number) {
    throw std::runtime_error("invalid args");
  }
  symbol_number_type result = args[0].number_val;
  for (auto it = args.cbegin() + 1; it != args.cend(); ++it) {
    const LispType &s = *it;
    if (s.type != LispType::Type::number) {
      throw std::runtime_error("symbol not a number");
    }
    result /= s.number_val;
  }
  result_sym.type = LispType::Type::number;
  result_sym.number_val = result;
}

void builtin_print(const std::vector<LispType> &args, LispType &result_sym)
{
  for (auto it = args.cbegin(); it != args.cend(); ++it) {
    const LispType &symbol = *it;
    print_lisp_type(symbol, false);
    if (it != args.cend() - 1) {
      std::cout << ", ";
    }
  }
  result_sym.type = LispType::Type::nil;
}

void builtin_exit(const std::vector<LispType> &args, LispType &result_sym)
{
  g_keep_running = false;
  result_sym = make_nil();
}

void init_builtins()
{
  g_builtins["+"] = builtin_add;
  g_builtins["-"] = builtin_min;
  g_builtins["/"] = builtin_div;
  g_builtins["*"] = builtin_mul;
  g_builtins["print"] = builtin_print;
  g_builtins["set"] = builtin_set;
  g_builtins["dump"] = builtin_dump_variables;
  g_builtins["cons"] = builtin_cons;
  g_builtins["car"] = builtin_car;
  g_builtins["cdr"] = builtin_cdr;
  g_builtins["nth"] = builtin_nth;
  g_builtins["list"] = builtin_list;
  g_builtins["exit"] = builtin_exit;
  //  g_builtins["quote"] = builtin_quite;
}

bool read_sexp(std::istringstream &input)
{
  return true;
}

LispType parse_lisp_type_from_symbol_name(const std::string &symbol_name)
{
  if (symbol_name.empty()) {
    throw std::runtime_error("cant parse empty symbol name");
  }

  if (symbol_name == "nil") {
    return LispType({
        .type = LispType::Type::nil
      });
  }
  
  bool escaped = *symbol_name.begin() == '\'';
  if (escaped) {
    return LispType({
        .type = LispType::Type::symbol,
        .string_val = symbol_name.substr(1)
      });
  }
  
  bool is_number = true;
  for (auto it = symbol_name.begin(); it != symbol_name.end(); ++it) {
    if (!std::isdigit(*it) && *it != '.' && *it != '-') {
      is_number = false;
    }
  }

  if (is_number) {
    return LispType({
        .type = LispType::Type::number,
        .number_val = std::stod(symbol_name)
      });
  } else {
    return LispType({
        .type = LispType::Type::variable,
        .string_val = symbol_name
      });
  }
}

struct Indent {
  int depth = 0;
};

static Indent g_indent;

std::ostream& operator<<(std::ostream& o, const Indent &indent)
{
  for (int i = 0; i < indent.depth; ++i) {
    o << " ";
  }
  return o;
}


void eval(const LispType& code, LispType &result, std::vector<LispType> &args)
{
  std::cout << g_indent << "eval: " << code << "\n";

  if (code.type == LispType::Type::nil) {
    result = make_nil();
    return;
  }

  if (code.type == LispType::Type::cons) {    
    LispType head = code.cons_val->head;
    std::cout << g_indent << "head: " << head << "\n";

    if (head.type == LispType::Type::cons) {
      std::cout << g_indent << "eval into head\n";
      // GOES INTO A FUNCTION
      std::vector<LispType> head_args;
      LispType return_val;
      g_indent.depth++;
      eval(head, return_val, head_args);
      g_indent.depth--;
      std::cout << g_indent << "return from head: " << return_val << "\n";
      // RETURN FROM A FUNCTION
      result = return_val;
    } else if (head.type != LispType::Type::function) {
      std::cout << g_indent << "GOT: ";
      print_lisp_type(head, true, std::cout);
      std::cout << "\n";
      result = head;
    }
    
    auto tail = code.cons_val->tail;

    if (tail != nullptr) {
      std::cout << g_indent << "eval into tail\n";
      LispType return_val;
      g_indent.depth++;
      eval(tail->head, return_val, args);
      g_indent.depth--;
      std::cout << g_indent << "return from tail: " << return_val << "\n";

      if (return_val.type != LispType::Type::nil) {
        std::cout << g_indent << "push arg: " << return_val << "\n";
        args.push_back(return_val);
      }
    }

    if (head.type == LispType::Type::function) {

      std::vector<LispType> resolved_vars;
      for (auto it = args.crbegin(); it != args.crend(); ++it) {
          if (it->type == LispType::Type::variable) {
            const LispType &resolved_var = g_variables[it->string_val];
            resolved_vars.push_back(resolved_var);
          } else {
            resolved_vars.push_back(*it);
          }
      }
      
      //      std::reverse(args.begin(), args.end());
      std::cout << g_indent << "FN CALL " << "(" << head;
      if (resolved_vars.size() > 0) {
        std::cout << " ";
      }
      for (auto it = resolved_vars.cbegin(); it != resolved_vars.cend(); ++it) {
        std::cout << *it;
        if (it != resolved_vars.cend() - 1) {
          std::cout << " ";
        }
      }
      std::cout << ")\n";

      auto func = g_builtins[head.string_val];
      func(resolved_vars, result);
    }
  } else {
    std::stringstream ss;
    ss << "eval_2. Type for code not implemented: " << code;
    throw std::runtime_error(ss.str());
  }
}

void eval(const LispType& code, LispType &result)
{
  std::vector<LispType> args;
  eval(code, result, args);
  std::cout << "RESULT: " << result << "\n";
}


void parse(std::string sexp, LispType &root)
{
  std::istringstream input(sexp);
  std::stringstream stream;
  std::string sym;
  bool got_func = false;
  
  int s_exp_cnt = 0;

  std::stack<LispType> func_stack;
  std::stack<std::vector<LispType>> args_stack;
  
  do {
    char token = input.get();
    switch (token) {
    case '(':
      got_func = false;
      s_exp_cnt++;
      break;
    case '\t':
    case ' ':
    case '\n':
    case ')':
      sym = stream.str();
      if (sym != "") {
        if (!got_func) {
          got_func = true;

          LispType new_func = {
            .type = LispType::Type::function,
            .string_val = sym
          };

          func_stack.push(new_func);
          
          args_stack.push({});
        } else {
          LispType parsed_type = parse_lisp_type_from_symbol_name(sym);
          args_stack.top().push_back(parsed_type);
        }
      }
      stream.str(std::string()); // clear

      if (token == ')') {
        LispType &func_type = func_stack.top();
        std::vector<LispType> &args = args_stack.top();

        LispType result = make_nil();
        for (auto it = args.crbegin(); it != args.crend(); ++it) {
          cons(*it, result, result);
        }

        cons(func_type, result, result);
        func_stack.pop();
        args_stack.pop();

        if (!args_stack.empty()) {
          args_stack.top().push_back(result);
        }

        --s_exp_cnt;
        if (s_exp_cnt == 0) {
          root = result;
        }
      }
      break;
    default:
      stream << token;
      break;
    }
  } while (input.good() && g_keep_running);

  if (s_exp_cnt != 0) {
    throw  std::runtime_error("unmatching number of ()");
  }
}

void parse_and_eval(const std::string &sexp, LispType &code, LispType &result)
{
  result = make_nil();
  parse(sexp, code);
  eval(code, result);
}

int main(int argc, char **argv)
{
  init_builtins();
  
  LispType code;
  LispType result;
  
  parse_and_eval("(+ 5.90  (-  10 2.1) (* 2 2))", code, result);

  assert(result.type == LispType::Type::number);
  assert(result.number_val == 17.8);

  parse_and_eval("(+ 5 3)", code, result);

  assert(result.type == LispType::Type::number);
  assert(result.number_val == 8.0);

  parse_and_eval("(+ (+ 3 (/ 8 3) (* (- 10 (+ 3 (* 2 (- 80 79))) 5) 8) (+ 7 (- 6 2))))", code, result);

  assert(result.type == LispType::Type::number);
  assert(result.number_val >= 16.6665 && result.number_val <= 16.6668);//6.9667);

  parse_and_eval("(set 'x 5)", code, result);
  parse_and_eval("(set 'y 3)", code, result);
  parse_and_eval("(* x y)", code, result);

  assert(result.type == LispType::Type::number);
  assert(result.number_val == 15);

  parse_and_eval("(set 'z (list 1 2 3))", code, result);

  assert(result.type == LispType::Type::cons);

  parse_and_eval("(nth 1 z)", code, result);

  assert(result.type == LispType::Type::number);
  assert(result.number_val == 2);

  parse_and_eval("(set 'z (list 1 (list 5 4 3 2 1)))", code, result);
  
  std::cout << "ALL TESTS PASSED!\n\nWelcome to MyLisp.\n";

  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0x0;

  sigaction(SIGINT, &sa, NULL);

  g_keep_running = true;
  
  while (g_keep_running) {
    std::cout << ">> ";
    std::cout.flush();
    std::string sexp;
    std::getline(std::cin, sexp);
    try {
      parse_and_eval(sexp, code, result);
      print_lisp_type(result, true);
      std::cout << "\n";
    } catch (const std::runtime_error &e) {
      std::cout << "Error: " << e.what() << "\n";
    }
  }
  
  return 0;
}
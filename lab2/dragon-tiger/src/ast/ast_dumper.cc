#include "ast_dumper.hh"
#include "../utils/errors.hh"

namespace {

char const * const get_type_name(ast::Type t) {
  switch (t) {
    case t_int:
      return "int";
    case t_string:
      return "string";
    default:
       utils::error("internal error: attempting to print the type of t_void or t_undef");
  }
}

} // namespace


namespace ast {
int32_t Evaluator::visit(const IntegerLiteral& literal) {
  result = literal.value;
  return result;
}

int32_t Evaluator::visit(const BinaryOperator& binop) {
  int lhs, rhs;
  binop.get_left().accept(*this);
  lhs = result;
  binop.get_right().accept(*this);
  rhs = result;

  switch (binop.op) {
    case types::Operator::o_plus:   result = lhs + rhs; break;
    case types::Operator::o_minus:  result = lhs - rhs; break;
    case types::Operator::o_times:  result = lhs * rhs; break;
    case types::Operator::o_divide:
      if (rhs == 0) utils::error("division by zero");
      result = lhs / rhs;
      break;
    case types::Operator::o_eq:     result = (lhs == rhs) ? 1 : 0; break;
    case types::Operator::o_lt:     result = (lhs < rhs) ? 1 : 0; break;
    case types::Operator::o_gt:     result = (lhs > rhs) ? 1 : 0; break;
    case types::Operator::o_le:     result = (lhs <= rhs) ? 1 : 0; break;
    case types::Operator::o_ge:     result = (lhs >= rhs) ? 1 : 0; break;
    default:
      utils::error("unsupported binary operator");
  }
  return result;
}

int32_t Evaluator::visit(const Sequence& seq) {
  const auto& exprs = seq.get_exprs();
  if (exprs.empty())
    utils::error("empty sequence expression");

  for (const auto& expr : exprs) {
    expr->accept(*this);
  }
  return result;
}

int32_t Evaluator::visit(const IfThenElse& ite) {
  ite.get_condition().accept(*this);
  if (result != 0) {
    ite.get_then_part().accept(*this);
  } else {
    ite.get_else_part().accept(*this);
  }
  return result;
}

// Unsupported expressions

int32_t Evaluator::visit(const StringLiteral&) {
  utils::error("cannot evaluate string literal");
  return result;
}

int32_t Evaluator::visit(const Identifier&) {
  utils::error("cannot evaluate identifier");
  return result;
}

int32_t Evaluator::visit(const Let&) {
  utils::error("cannot evaluate let expression");
  return result;
}

int32_t Evaluator::visit(const FunCall&) {
  utils::error("cannot evaluate function call");
  return result;
}

int32_t Evaluator::visit(const WhileLoop&) {
  utils::error("cannot evaluate while loop");
  return result;
}

int32_t Evaluator::visit(const ForLoop&) {
  utils::error("cannot evaluate for loop");
  return result;
}

int32_t Evaluator::visit(const Break&) {
  utils::error("cannot evaluate break statement");
  return result;
}

int32_t Evaluator::visit(const Assign&) {
  utils::error("cannot evaluate assignment");
  return result;
}

int32_t Evaluator::visit(const VarDecl&) {
  utils::error("cannot evaluate variable declaration");
  return result;
}

int32_t Evaluator::visit(const FunDecl&) {
  utils::error("cannot evaluate function declaration");
  return result;
}
void ASTDumper::visit(const IntegerLiteral &literal) {
  *ostream << literal.value;
}

void ASTDumper::visit(const StringLiteral &literal) {
  *ostream << '"';
  for (auto &c : static_cast<std::string>(literal.value)) {
    switch (c) {
    case '"':
      *ostream << "\\\"";
      break;
    case '\\':
      *ostream << "\\\\";
      break;
    case '\a':
      *ostream << "\\a";
      break;
    case '\b':
      *ostream << "\\b";
      break;
    case '\t':
      *ostream << "\\t";
      break;
    case '\n':
      *ostream << "\\n";
      break;
    case '\v':
      *ostream << "\\v";
      break;
    case '\f':
      *ostream << "\\f";
      break;
    case '\r':
      *ostream << "\\r";
      break;
    default:
      *ostream << c;
    }
  }
  *ostream << '"';
}

void ASTDumper::visit(const BinaryOperator &binop) {
  *ostream << '(';
  binop.get_left().accept(*this);
  *ostream << operator_name[binop.op];
  binop.get_right().accept(*this);
  *ostream << ')';
}

void ASTDumper::visit(const Sequence &seqExpr) {
  *ostream << "(";
  inc();
  const auto exprs = seqExpr.get_exprs();
  for (auto expr = exprs.cbegin(); expr != exprs.cend(); expr++) {
    if (expr != exprs.cbegin())
      *ostream << ';';
    nl();
    (*expr)->accept(*this);
  }
  dnl();
  *ostream << ")";
}

void ASTDumper::visit(const Let &let) {
  *ostream << "let";
  inc();
  for (auto decl : let.get_decls()) {
    nl();
    decl->accept(*this);
  }
  dnl();
  *ostream << "in";
  inc();
  const auto exprs = let.get_sequence().get_exprs();
  for (auto expr = exprs.cbegin(); expr != exprs.cend(); expr++) {
    if (expr != exprs.cbegin())
      *ostream << ';';
    nl();
    (*expr)->accept(*this);
  }
  dnl();
  *ostream << "end";
}

void ASTDumper::visit(const Identifier &id) {
  *ostream << id.name;
  if (verbose)
    if (auto decl = id.get_decl()) {
      *ostream << "/*" << "decl:" << decl.get().loc;

      if (int depth_diff = id.get_depth() - decl->get_depth())
        *ostream << " depth_diff:" << depth_diff;
      *ostream << "*/";
    }
}

void ASTDumper::visit(const IfThenElse &ite) {
  *ostream << "if ";
  inl();
  ite.get_condition().accept(*this);
  dnl();
  *ostream << " then ";
  inl();
  ite.get_then_part().accept(*this);
  dnl();
  *ostream << " else ";
  inl();
  ite.get_else_part().accept(*this);
  dec();
}

void ASTDumper::visit(const VarDecl &decl) {
  if (decl.get_expr())
    *ostream << "var ";
  *ostream << decl.name;
  if (verbose && decl.get_escapes())
    *ostream << "/*e*/";
  if (decl.type_name)
    *ostream << ": " << *decl.type_name;
  else {
    auto t = decl.get_type();
    if (t != t_undef && t != t_void)
      *ostream << ": " << get_type_name(t);
  }
  if (auto expr = decl.get_expr()) {
    *ostream << " := ";
    expr->accept(*this);
  }
}

void ASTDumper::visit(const FunDecl &decl) {
  *ostream << "function " << decl.name;
  if (verbose && decl.name != decl.get_external_name())
    *ostream << "/*" << decl.get_external_name() << "*/";
  *ostream << '(';
  auto params = decl.get_params();
  for (auto param = params.cbegin(); param != params.cend(); param++) {
    if (param != params.cbegin())
      *ostream << ", ";
    (*param)->accept(*this);
  }
  *ostream << ")";
  if (decl.type_name)
    *ostream << ": " << decl.type_name.get();
  *ostream << " = ";
  inl();
  decl.get_expr()->accept(*this);
  dec();
}

void ASTDumper::visit(const FunCall &call) {
  *ostream << call.func_name;
  if (verbose)
    if (auto decl = call.get_decl())
      *ostream << "/*" << "decl:" << decl.get().loc << "*/";

  *ostream << "(";

  auto args = call.get_args();
  for (auto arg = args.cbegin(); arg != args.cend(); arg++) {
    if (arg != args.cbegin())
      *ostream << ", ";
    (*arg)->accept(*this);
  }
  *ostream << ')';
}

void ASTDumper::visit(const WhileLoop &loop) {
  *ostream << "while ";
  loop.get_condition().accept(*this);
  *ostream << " do";
  inl();
  loop.get_body().accept(*this);
  dec();
}

void ASTDumper::visit(const ForLoop &loop) {
  *ostream << "for " << loop.get_variable().name;
  if (verbose && loop.get_variable().get_escapes())
    *ostream << "/*e*/";
  *ostream << " := ";
  loop.get_variable().get_expr()->accept(*this);
  *ostream << " to ";
  loop.get_high().accept(*this);
  *ostream << " do";
  inl();
  loop.get_body().accept(*this);
  dec();
}

void ASTDumper::visit(const Break &brk) {
  *ostream << "break";
  if (verbose && brk.get_loop())
    *ostream << "/*loop:" << brk.get_loop().get().loc << "*/";
}

void ASTDumper::visit(const Assign &assign) {
  assign.get_lhs().accept(*this);
  *ostream << " := ";
  assign.get_rhs().accept(*this);
}

} // namespace ast

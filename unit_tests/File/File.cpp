﻿#include <neolib/neolib.hpp>
#include <neolib/core/i_enum.hpp>
#include <neolib/file/lexer.hpp>

namespace
{
    void test_assert(bool assertion)
    {
        if (!assertion)
            throw std::logic_error("Test failed");
    }
    void test_assert(int iteration, bool assertion)
    {
        if (!assertion)
        {
            std::cerr << "Test failed, iteration = " << iteration << std::endl;
            throw std::logic_error("Test failed");
        }
    }
}

namespace lexer_test
{
    enum class token
    {
        Program,
        Whitespace,
        Eof,
        Identifier,
        FunctionDefinition,
        FunctionPrototype,
        FunctionBody,
        FunctionReturnType,
        FunctionName,
        FunctionParameterList,
        FunctionParameterListOpen,
        FunctionParameterListClose,
        FunctionParameter,
        OpenScope,
        CloseScope,
        Type,
        Statement,
        EndStatement,
        Expression,
        OpenExpression,
        CloseExpression,
        Term,
        Factor,
        Primary,
        Add,
        Subtract,
        Multiply,
        Divide,
        Negate,
        Number,
        Digit,
        Decimal,
        Variable,
        Assign,
        Equal,
    };
}

declare_tokens(lexer_test::token)
declare_token(lexer_test::token, Program)
declare_token(lexer_test::token, Whitespace)
declare_token(lexer_test::token, Eof)
declare_token(lexer_test::token, Identifier)
declare_token(lexer_test::token, FunctionDefinition)
declare_token(lexer_test::token, FunctionPrototype)
declare_token(lexer_test::token, FunctionBody)
declare_token(lexer_test::token, FunctionReturnType)
declare_token(lexer_test::token, FunctionName)
declare_token(lexer_test::token, FunctionParameterList)
declare_token(lexer_test::token, FunctionParameterListOpen)
declare_token(lexer_test::token, FunctionParameterListClose)
declare_token(lexer_test::token, FunctionParameter)
declare_token(lexer_test::token, OpenScope)
declare_token(lexer_test::token, CloseScope)
declare_token(lexer_test::token, Type)
declare_token(lexer_test::token, Statement)
declare_token(lexer_test::token, EndStatement)
declare_token(lexer_test::token, Expression)
declare_token(lexer_test::token, OpenExpression)
declare_token(lexer_test::token, CloseExpression)
declare_token(lexer_test::token, Term)
declare_token(lexer_test::token, Factor)
declare_token(lexer_test::token, Primary)
declare_token(lexer_test::token, Add)
declare_token(lexer_test::token, Subtract)
declare_token(lexer_test::token, Multiply)
declare_token(lexer_test::token, Divide)
declare_token(lexer_test::token, Negate)
declare_token(lexer_test::token, Number)
declare_token(lexer_test::token, Digit)
declare_token(lexer_test::token, Decimal)
declare_token(lexer_test::token, Variable)
declare_token(lexer_test::token, Assign)
declare_token(lexer_test::token, Equal)
end_declare_tokens(lexer_test::token);

namespace lexer_test
{
    enable_neolib_lexer(token)
}

std::string_view const sourcePass1 = R"test(r f(){})test";

std::string_view const sourcePass2 = R"test(
    xyzzY0 foo()
    {
        1234;
        x := 1 + 2 + 3 - 4 - 5 + 6; 
        y := 7 + -42.001 * 1.0 * (5-1+2) + x * 2;
    }
)test";

std::string_view const sourceError1 = R"test(
    xyzzY0 foo()
    {
        1234q;
        x := 1 + 2 + 3 - 4 - 5 + 6; 
        y := 7 + -42.001 * 1.0 * (5-1+2) + x * 2;
    }
)test";

std::string_view const sourceError2 = R"test(
    xyzzY0 foo()
    {
        1234;
        x := 1 + 2 + 3 - 4 - 5 + 6; 
        y := 7 + 4
2.0 * 1.0 * (5-1+2) + x * 2;
    }
)test";

std::string_view const sourceError3 = R"test(
    xyzzY0 foo()
    {
        1234;
        x := 1 + 2 + 3 - 4 - 5 + 6; 
        y := 7 + -42.001 * 1.0 * (5-1+2)) + x * 2;
    }
)test";

int main(int argc, char** argv)
{
    using namespace lexer_test;

    neolib::lexer_rule<token> lexerRules[] =
    {
        ( token::Program >> repeat(token::FunctionDefinition) , discard(token::Eof)),
        ( token::FunctionDefinition >> token::FunctionPrototype , token::FunctionBody ),
        ( token::FunctionPrototype >> token::FunctionReturnType , token::FunctionName , token::FunctionParameterList ),
        ( token::FunctionReturnType >> token::Type ),
        ( token::FunctionName >> token::Identifier ),
        ( token::FunctionParameterList >> 
            ~discard(token::FunctionParameterListOpen) , 
            optional(sequence(token::FunctionParameter, repeat(sequence(',' , token::FunctionParameter)))) , 
            ~discard(token::FunctionParameterListClose) ),
        ( token::FunctionParameterListOpen >> '(' ),
        ( token::FunctionParameterListClose >> ')' ),
        ( token::FunctionParameter >> token::Type, token::Variable ),
        ( token::FunctionBody >> (~discard(token::OpenScope) , repeat(token::Statement) , ~discard(token::CloseScope)) ),
        ( token::Type >> token::Identifier ),
        ( token::Identifier >> (+(range('A', 'Z') | range('a', 'z')) , 
            (range('A', 'Z') | range('a', 'z') | range('0', '9'))) ),
        ( token::OpenScope >> '{' ),
        ( token::CloseScope >> '}' ),
        ( token::Statement >> token::Expression , discard(token::EndStatement) ),
        ( token::EndStatement >> ';' ),
        ( token::Expression >> token::Term , 
            +repeat((choice(
                token::Add <=> "math.operator.add"_concept_associate_left | 
                token::Subtract <=> "math.operator.subtract"_concept_associate_left), token::Term)) ),
        ( token::Expression >> token::Term ),
        ( token::Term >> token::Factor ,
            +repeat((choice(
                token::Multiply <=> "math.operator.multiply"_concept_associate_left | 
                token::Divide <=> "math.operator.divide"_concept_associate_left), token::Factor)) ),
        ( token::Term >> token::Factor ),
        ( token::Factor >> token::Primary ),
        ( token::Primary >> 
            ((token::Variable <=> "object"_concept , token::Assign , token::Expression) <=> "object.assign"_concept)),
        ( token::Primary >> ((token::Negate , token::Primary) <=> "math.operator.negate"_concept) ),
        ( token::Primary >> token::Number ),
        ( token::Primary >> (token::Variable <=> "object"_concept) ),
        ( token::Primary >> ~discard(token::OpenExpression) , token::Expression , ~discard(token::CloseExpression) ),
        ( token::OpenExpression >> '(' ),
        ( token::CloseExpression >> ')' ),
        ( token::Add >> '+' ),
        ( token::Subtract >> '-' ),
        ( token::Multiply >> '*' ),
        ( token::Divide >> '/' ),
        ( token::Negate >> '-' ),
        ( token::Assign >> ":=" ),
        ( token::Equal >> '=' ),
        ( token::Number >> (fold((+repeat(token::Digit) , token::Decimal, +repeat(token::Digit))) <=> "number.float"_concept) ),
        ( token::Number >> (fold((+repeat(token::Digit))) <=> "number.integer"_concept) ),
        ( token::Digit >> range('0' , '9') ),
        ( token::Decimal >> '.' ),
        ( token::Variable >> token::Identifier ),

        // whitespace handling...

        ( token::Eof >> discard(token::Whitespace), "" ),
        ( token::Whitespace >> (' '_ | '\r' | '\n' | '\t') ),
        ( token::Program >> discard(token::Whitespace) , token::Program , discard(token::Whitespace) ),
        ( token::FunctionDefinition >> discard(token::Whitespace) , token::FunctionDefinition , discard(token::Whitespace) ),
        ( token::FunctionPrototype >> discard(token::Whitespace) , token::FunctionPrototype , discard(token::Whitespace) ),
        ( token::FunctionBody >> discard(token::Whitespace) , token::FunctionBody , discard(token::Whitespace) ),
        ( token::FunctionReturnType >> discard(token::Whitespace) , token::FunctionReturnType , discard(token::Whitespace) ),
        ( token::FunctionName >> discard(token::Whitespace) , token::FunctionName , discard(token::Whitespace) ),
        ( token::FunctionParameter >> discard(token::Whitespace) , token::FunctionParameter , discard(token::Whitespace) ),
        ( token::OpenScope >> discard(token::Whitespace) , token::OpenScope , discard(token::Whitespace) ),
        ( token::CloseScope >> discard(token::Whitespace) , token::CloseScope , discard(token::Whitespace) ),
        ( token::Statement >> discard(token::Whitespace) , token::Statement , discard(token::Whitespace) ),
        ( token::EndStatement >> discard(token::Whitespace) , token::EndStatement , discard(token::Whitespace) ),
        ( token::Expression >> discard(token::Whitespace) , token::Expression , discard(token::Whitespace) ),
        ( token::OpenExpression >> discard(token::Whitespace) , token::OpenExpression , discard(token::Whitespace) ),
        ( token::CloseExpression >> discard(token::Whitespace) , token::CloseExpression , discard(token::Whitespace) ),
        ( token::Variable >> discard(token::Whitespace) , token::Variable , discard(token::Whitespace) ),
        ( token::Identifier >> discard(token::Whitespace) , token::Identifier , discard(token::Whitespace) ),
        ( token::Number >> discard(token::Whitespace) , token::Number, discard(token::Whitespace) ),
        ( token::Assign >> discard(token::Whitespace) , token::Assign, discard(token::Whitespace) ),
        ( token::Equal >> discard(token::Whitespace) , token::Equal, discard(token::Whitespace) ),
        ( token::Add >> discard(token::Whitespace) , token::Add, discard(token::Whitespace) ),
        ( token::Subtract >> discard(token::Whitespace) , token::Subtract, discard(token::Whitespace) ),
        ( token::Multiply >> discard(token::Whitespace) , token::Multiply, discard(token::Whitespace) ),
        ( token::Divide >> discard(token::Whitespace) , token::Divide, discard(token::Whitespace) ),
        ( token::Negate >> discard(token::Whitespace) , token::Negate, discard(token::Whitespace) ),
        ( token::Term >> discard(token::Whitespace) , token::Term , discard(token::Whitespace) ),
        ( token::Factor >> discard(token::Whitespace) , token::Factor , discard(token::Whitespace) ),
        ( token::Primary >> discard(token::Whitespace) , token::Primary , discard(token::Whitespace) )
    };

    neolib::lexer<token> lexer{ lexerRules };
    lexer.set_debug_output(std::cerr);
    lexer.set_debug_scan(false);
    test_assert(lexer.parse(token::Program, sourcePass1));
    test_assert(lexer.parse(token::Program, sourcePass2));
    lexer.create_ast();
    test_assert(!lexer.parse(token::Program, sourceError1));
    test_assert(!lexer.parse(token::Program, sourceError2));
    test_assert(!lexer.parse(token::Program, sourceError3));
}


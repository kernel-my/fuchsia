// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// lazy_static is required for re_find_static.
use lazy_static::lazy_static;
use nom::{
    branch::alt,
    bytes::complete::{escaped, is_not, tag},
    character::complete::multispace0,
    character::complete::{char, digit1, hex_digit1, one_of},
    combinator::{map, map_res, opt, value},
    error::{ErrorKind, ParseError},
    multi::{many0, separated_nonempty_list},
    named, re_find_static,
    sequence::{delimited, preceded},
    IResult,
};

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct CompoundIdentifier {
    pub namespace: Vec<String>,
    pub name: String,
}

impl CompoundIdentifier {
    pub fn nest(&self, name: String) -> CompoundIdentifier {
        let mut namespace = self.namespace.clone();
        namespace.push(self.name.clone());
        CompoundIdentifier { namespace, name }
    }

    pub fn parent(&self) -> Option<CompoundIdentifier> {
        let mut namespace = self.namespace.clone();
        let name = namespace.pop()?;
        Some(CompoundIdentifier { namespace, name })
    }
}

impl ToString for CompoundIdentifier {
    fn to_string(&self) -> String {
        let mut temp = self.namespace.clone();
        temp.push(self.name.clone());
        temp.join(".")
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct Include {
    pub name: CompoundIdentifier,
    pub alias: Option<String>,
}

#[derive(Debug, PartialEq)]
pub enum BindParserError {
    Type(String),
    StringLiteral(String),
    NumericLiteral(String),
    BoolLiteral(String),
    Identifier(String),
    Semicolon(String),
    Assignment(String),
    ListStart(String),
    ListEnd(String),
    ListSeparator(String),
    LibraryKeyword(String),
    UsingKeyword(String),
    AsKeyword(String),
    IfBlockStart(String),
    IfBlockEnd(String),
    IfKeyword(String),
    ElseKeyword(String),
    ConditionOp(String),
    ConditionValue(String),
    AcceptKeyword(String),
    UnrecognisedInput(String),
    Unknown(String, ErrorKind),
}

impl ParseError<&str> for BindParserError {
    fn from_error_kind(input: &str, kind: ErrorKind) -> Self {
        BindParserError::Unknown(input.to_string(), kind)
    }

    fn append(_input: &str, _kind: ErrorKind, e: Self) -> Self {
        e
    }
}

pub fn string_literal(input: &str) -> IResult<&str, String, BindParserError> {
    let escapable = escaped(is_not(r#"\""#), '\\', one_of(r#"\""#));
    let literal = delimited(char('"'), escapable, char('"'));
    map_err(map(literal, |s: &str| s.to_string()), BindParserError::StringLiteral)(input)
}

pub fn numeric_literal(input: &str) -> IResult<&str, u64, BindParserError> {
    let base10 = map_res(digit1, |s| u64::from_str_radix(s, 10));
    let base16 = map_res(preceded(tag("0x"), hex_digit1), |s| u64::from_str_radix(s, 16));
    // Note: When the base16 parser fails but input starts with '0x' this will succeed and return 0.
    map_err(alt((base16, base10)), BindParserError::NumericLiteral)(input)
}

pub fn bool_literal(input: &str) -> IResult<&str, bool, BindParserError> {
    let true_ = value(true, tag("true"));
    let false_ = value(false, tag("false"));
    map_err(alt((true_, false_)), BindParserError::BoolLiteral)(input)
}

pub fn identifier(input: &str) -> IResult<&str, String, BindParserError> {
    named!(matcher<&str,&str>, re_find_static!(r"^[a-zA-Z]([a-zA-Z0-9_]*[a-zA-Z0-9])?"));
    map_err(map(matcher, |s| s.to_string()), BindParserError::Identifier)(input)
}

pub fn compound_identifier(input: &str) -> IResult<&str, CompoundIdentifier, BindParserError> {
    let (input, mut segments) = separated_nonempty_list(tag("."), identifier)(input)?;
    // Segments must be nonempty, so it's safe to pop off the name.
    let name = segments.pop().unwrap();
    Ok((input, CompoundIdentifier { namespace: segments, name }))
}

pub fn using(input: &str) -> IResult<&str, Include, BindParserError> {
    let as_keyword = map_err(ws(tag("as")), BindParserError::AsKeyword);
    let (input, name) = compound_identifier(input)?;
    let (input, alias) = opt(preceded(as_keyword, identifier))(input)?;
    Ok((input, Include { name, alias }))
}

pub fn using_list(input: &str) -> IResult<&str, Vec<Include>, BindParserError> {
    let using_keyword = map_err(ws(tag("using")), BindParserError::UsingKeyword);
    let terminator = map_err(ws(tag(";")), BindParserError::Semicolon);
    let using = delimited(using_keyword, ws(using), terminator);
    many0(ws(using))(input)
}

// Reimplementation of nom::combinator::all_consuming with BindParserError error type.
pub fn all_consuming<'a, O, F>(f: F) -> impl Fn(&'a str) -> IResult<&'a str, O, BindParserError>
where
    F: Fn(&'a str) -> IResult<&'a str, O, BindParserError>,
{
    move |input: &'a str| {
        let (input, res) = f(input)?;
        if input.len() == 0 {
            Ok((input, res))
        } else {
            Err(nom::Err::Error(BindParserError::UnrecognisedInput(input.to_string())))
        }
    }
}

// Wraps a parser |f| and discards zero or more whitespace characters before and after it.
pub fn ws<I, O, E: ParseError<I>, F>(f: F) -> impl Fn(I) -> IResult<I, O, E>
where
    I: nom::InputTakeAtPosition<Item = char>,
    F: Fn(I) -> IResult<I, O, E>,
{
    delimited(multispace0, f, multispace0)
}

// Wraps a parser and replaces its error.
pub fn map_err<'a, O, P, G>(
    parser: P,
    f: G,
) -> impl Fn(&'a str) -> IResult<&'a str, O, BindParserError>
where
    P: Fn(&'a str) -> IResult<&'a str, O, (&'a str, ErrorKind)>,
    G: Fn(String) -> BindParserError,
{
    move |input: &str| {
        parser(input).map_err(|e| match e {
            nom::Err::Error((i, _)) => nom::Err::Error(f(i.to_string())),
            nom::Err::Failure((i, _)) => nom::Err::Failure(f(i.to_string())),
            nom::Err::Incomplete(_) => {
                unreachable!("Parser should never generate Incomplete errors")
            }
        })
    }
}

#[macro_export]
macro_rules! make_identifier{
    ( $name:tt ) => {
        {
            CompoundIdentifier {
                namespace: Vec::new(),
                name: String::from($name),
            }
        }
    };
    ( $namespace:tt , $($rest:tt)* ) => {
        {
            let mut identifier = make_identifier!($($rest)*);
            identifier.namespace.insert(0, String::from($namespace));
            identifier
        }
    };
}

#[cfg(test)]
mod test {
    use super::*;

    mod string_literal {
        use super::*;

        #[test]
        fn basic() {
            // Matches a string literal, leaves correct tail.
            assert_eq!(string_literal(r#""abc 123"xyz"#), Ok(("xyz", "abc 123".to_string())));
        }

        #[test]
        fn match_once() {
            assert_eq!(string_literal(r#""abc""123""#), Ok((r#""123""#, "abc".to_string())));
        }

        #[test]
        fn escaped() {
            assert_eq!(
                string_literal(r#""abc \"esc\" xyz""#),
                Ok(("", r#"abc \"esc\" xyz"#.to_string()))
            );
        }

        #[test]
        fn requires_quotations() {
            assert_eq!(
                string_literal(r#"abc"#),
                Err(nom::Err::Error(BindParserError::StringLiteral("abc".to_string())))
            );
        }

        #[test]
        fn empty() {
            assert_eq!(
                string_literal(""),
                Err(nom::Err::Error(BindParserError::StringLiteral("".to_string())))
            );
        }
    }

    mod numeric_literals {
        use super::*;

        #[test]
        fn decimal() {
            assert_eq!(numeric_literal("123"), Ok(("", 123)));
        }

        #[test]
        fn hex() {
            assert_eq!(numeric_literal("0x123"), Ok(("", 0x123)));
            assert_eq!(numeric_literal("0xabcdef"), Ok(("", 0xabcdef)));
            assert_eq!(numeric_literal("0xABCDEF"), Ok(("", 0xabcdef)));
            assert_eq!(numeric_literal("0x123abc"), Ok(("", 0x123abc)));

            // Does not match hex without '0x' prefix.
            assert_eq!(
                numeric_literal("abc123"),
                Err(nom::Err::Error(BindParserError::NumericLiteral("abc123".to_string())))
            );
            assert_eq!(numeric_literal("123abc"), Ok(("abc", 123)));
        }

        #[test]
        fn non_numbers() {
            assert_eq!(
                numeric_literal("xyz"),
                Err(nom::Err::Error(BindParserError::NumericLiteral("xyz".to_string())))
            );

            // Does not match negative numbers (for now).
            assert_eq!(
                numeric_literal("-1"),
                Err(nom::Err::Error(BindParserError::NumericLiteral("-1".to_string())))
            );
        }

        #[test]
        fn overflow() {
            // Does not match numbers larger than u64.
            assert_eq!(numeric_literal("18446744073709551615"), Ok(("", 18446744073709551615)));
            assert_eq!(numeric_literal("0xffffffffffffffff"), Ok(("", 18446744073709551615)));
            assert_eq!(
                numeric_literal("18446744073709551616"),
                Err(nom::Err::Error(BindParserError::NumericLiteral(
                    "18446744073709551616".to_string()
                )))
            );
            // Note: This is matching '0' from '0x' but failing to parse the entire string.
            assert_eq!(numeric_literal("0x10000000000000000"), Ok(("x10000000000000000", 0)));
        }

        #[test]
        fn empty() {
            // Does not match an empty string.
            assert_eq!(
                numeric_literal(""),
                Err(nom::Err::Error(BindParserError::NumericLiteral("".to_string())))
            );
        }
    }

    mod bool_literals {
        use super::*;

        #[test]
        fn basic() {
            assert_eq!(bool_literal("true"), Ok(("", true)));
            assert_eq!(bool_literal("false"), Ok(("", false)));
        }

        #[test]
        fn non_bools() {
            // Does not match anything else.
            assert_eq!(
                bool_literal("tralse"),
                Err(nom::Err::Error(BindParserError::BoolLiteral("tralse".to_string())))
            );
        }

        #[test]
        fn empty() {
            // Does not match an empty string.
            assert_eq!(
                bool_literal(""),
                Err(nom::Err::Error(BindParserError::BoolLiteral("".to_string())))
            );
        }
    }

    mod identifiers {
        use super::*;

        #[test]
        fn basic() {
            // Matches identifiers with lowercase, uppercase, digits, and underscores.
            assert_eq!(identifier("abc_123_ABC"), Ok(("", "abc_123_ABC".to_string())));

            // Match is terminated by whitespace or punctuation.
            assert_eq!(identifier("abc_123_ABC "), Ok((" ", "abc_123_ABC".to_string())));
            assert_eq!(identifier("abc_123_ABC;"), Ok((";", "abc_123_ABC".to_string())));
        }

        #[test]
        fn invalid() {
            // Does not match an identifier beginning or ending with '_'.
            assert_eq!(
                identifier("_abc"),
                Err(nom::Err::Error(BindParserError::Identifier("_abc".to_string())))
            );

            // Note: Matches up until the '_' but fails to parse the entire string.
            assert_eq!(identifier("abc_"), Ok(("_", "abc".to_string())));
        }

        #[test]
        fn empty() {
            // Does not match an empty string.
            assert_eq!(
                identifier(""),
                Err(nom::Err::Error(BindParserError::Identifier("".to_string())))
            );
        }
    }

    mod compound_identifiers {
        use super::*;

        #[test]
        fn single_identifier() {
            // Matches single identifier.
            assert_eq!(
                compound_identifier("abc_123_ABC"),
                Ok(("", make_identifier!["abc_123_ABC"]))
            );
        }

        #[test]
        fn multiple_identifiers() {
            // Matches compound identifiers.
            assert_eq!(compound_identifier("abc.def"), Ok(("", make_identifier!["abc", "def"])));
            assert_eq!(
                compound_identifier("abc.def.ghi"),
                Ok(("", make_identifier!["abc", "def", "ghi"]))
            );
        }

        #[test]
        fn empty() {
            // Does not match empty identifiers.
            assert_eq!(
                compound_identifier("."),
                Err(nom::Err::Error(BindParserError::Identifier(".".to_string())))
            );
            assert_eq!(
                compound_identifier(".abc"),
                Err(nom::Err::Error(BindParserError::Identifier(".abc".to_string())))
            );
            assert_eq!(compound_identifier("abc..def"), Ok(("..def", make_identifier!["abc"])));
            assert_eq!(compound_identifier("abc."), Ok((".", make_identifier!["abc"])));

            // Does not match an empty string.
            assert_eq!(
                compound_identifier(""),
                Err(nom::Err::Error(BindParserError::Identifier("".to_string())))
            );
        }
    }

    mod using_lists {
        use super::*;

        #[test]
        fn one_include() {
            assert_eq!(
                using_list("using test;"),
                Ok(("", vec![Include { name: make_identifier!["test"], alias: None }]))
            );
        }

        #[test]
        fn multiple_includes() {
            assert_eq!(
                using_list("using abc;using def;"),
                Ok((
                    "",
                    vec![
                        Include { name: make_identifier!["abc"], alias: None },
                        Include { name: make_identifier!["def"], alias: None },
                    ]
                ))
            );
            assert_eq!(
                using_list("using abc;using def;using ghi;"),
                Ok((
                    "",
                    vec![
                        Include { name: make_identifier!["abc"], alias: None },
                        Include { name: make_identifier!["def"], alias: None },
                        Include { name: make_identifier!["ghi"], alias: None },
                    ]
                ))
            );
        }

        #[test]
        fn compound_identifiers() {
            assert_eq!(
                using_list("using abc.def;"),
                Ok(("", vec![Include { name: make_identifier!["abc", "def"], alias: None }]))
            );
        }

        #[test]
        fn aliases() {
            assert_eq!(
                using_list("using abc.def as one;using ghi as two;"),
                Ok((
                    "",
                    vec![
                        Include {
                            name: make_identifier!["abc", "def"],
                            alias: Some("one".to_string()),
                        },
                        Include { name: make_identifier!["ghi"], alias: Some("two".to_string()) },
                    ]
                ))
            );
        }

        #[test]
        fn whitespace() {
            assert_eq!(
                using_list(" using   abc\t as  one  ;\n using def ; "),
                Ok((
                    "",
                    vec![
                        Include { name: make_identifier!["abc"], alias: Some("one".to_string()) },
                        Include { name: make_identifier!["def"], alias: None },
                    ]
                ))
            );
            assert_eq!(
                using_list("usingabc;"),
                Ok(("", vec![Include { name: make_identifier!["abc"], alias: None }]))
            );
        }

        #[test]
        fn invalid() {
            // Must be followed by ';'.
            assert_eq!(using_list("using abc"), Ok(("using abc", vec![])));
        }

        #[test]
        fn empty() {
            assert_eq!(using_list(""), Ok(("", vec![])));
        }
    }
}

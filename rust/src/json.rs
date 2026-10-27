//! Minimal JSON support for the verifier contract: a small recursive-descent
//! parser over flat objects (with one nesting level for the summary map).
//! Dependency-free on purpose — the harness must not drag a serde stack
//! into a zero-dependency crate.

use std::collections::BTreeMap;

#[derive(Debug, Clone, PartialEq)]
pub enum Value {
    Num(f64),
    Str(String),
    Obj(BTreeMap<String, Value>),
}

pub fn parse_object(line: &str) -> Result<BTreeMap<String, Value>, String> {
    let mut p = Parser { src: line.trim(), pos: 0 };
    p.ws();
    let obj = p.object()?;
    p.ws();
    if p.pos != p.src.len() {
        return Err(format!("trailing characters at {}", p.pos));
    }
    Ok(obj)
}

struct Parser<'a> {
    src: &'a str,
    pos: usize,
}

impl<'a> Parser<'a> {
    fn object(&mut self) -> Result<BTreeMap<String, Value>, String> {
        self.expect('{')?;
        let mut map = BTreeMap::new();
        self.ws();
        if self.peek()? == '}' {
            self.pos += 1;
            return Ok(map);
        }
        loop {
            self.ws();
            let key = self.string()?;
            self.ws();
            self.expect(':')?;
            self.ws();
            let value = self.value()?;
            map.insert(key, value);
            self.ws();
            let c = self.next()?;
            if c == '}' {
                return Ok(map);
            }
            if c != ',' {
                return Err(format!("expected ',' or '}}' at {}", self.pos - 1));
            }
        }
    }

    fn value(&mut self) -> Result<Value, String> {
        match self.peek()? {
            '{' => Ok(Value::Obj(self.object()?)),
            '"' => Ok(Value::Str(self.string()?)),
            _ => self.number(),
        }
    }

    fn number(&mut self) -> Result<Value, String> {
        let start = self.pos;
        while self.pos < self.src.len() && b"-+.eE0123456789".contains(&self.src.as_bytes()[self.pos]) {
            self.pos += 1;
        }
        if start == self.pos {
            return Err(format!("expected a number at {}", self.pos));
        }
        self.src[start..self.pos]
            .parse::<f64>()
            .map(Value::Num)
            .map_err(|e| e.to_string())
    }

    fn string(&mut self) -> Result<String, String> {
        self.expect('"')?;
        let mut out = String::new();
        loop {
            let c = self.next()?;
            match c {
                '"' => return Ok(out),
                '\\' => {
                    let e = self.next()?;
                    match e {
                        '"' => out.push('"'),
                        '\\' => out.push('\\'),
                        '/' => out.push('/'),
                        'n' => out.push('\n'),
                        'r' => out.push('\r'),
                        't' => out.push('\t'),
                        'b' => out.push('\u{8}'),
                        'f' => out.push('\u{c}'),
                        'u' => {
                            let hex = &self.src[self.pos..self.pos + 4];
                            self.pos += 4;
                            let cp = u32::from_str_radix(hex, 16).map_err(|e| e.to_string())?;
                            out.push(char::from_u32(cp).unwrap_or('\u{fffd}'));
                        }
                        _ => return Err(format!("bad escape \\{e}")),
                    }
                }
                _ => out.push(c),
            }
        }
    }

    fn ws(&mut self) {
        while self.pos < self.src.len() && self.src.as_bytes()[self.pos].is_ascii_whitespace() {
            self.pos += 1;
        }
    }

    fn peek(&self) -> Result<char, String> {
        self.src[self.pos..]
            .chars()
            .next()
            .ok_or_else(|| "unexpected end of input".to_string())
    }

    fn next(&mut self) -> Result<char, String> {
        let c = self.peek()?;
        self.pos += c.len_utf8();
        Ok(c)
    }

    fn expect(&mut self, c: char) -> Result<(), String> {
        if self.peek()? != c {
            return Err(format!("expected '{c}' at {}", self.pos));
        }
        self.pos += 1;
        Ok(())
    }
}

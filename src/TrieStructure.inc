#include <Token.hpp>
#include <Trie.hpp>
const TrieNode trie[] = {
    TrieNode('!', 1, 3, TokenType::Not, true, true),           // 0
    TrieNode('<', TokenType::NotSmaller),                      // 1
    TrieNode('=', TokenType::NotEqual),                        // 2
    TrieNode('>', TokenType::NotBigger),                       // 3
    TrieNode('%', 5, 1, TokenType::Percent, true, true),       // 4
    TrieNode('=', TokenType::PercentEqual),                    // 5
    TrieNode('&', 7, 1, TokenType::Ampersand, true, true),     // 6
    TrieNode('=', TokenType::AmpersandEquals),                 // 7
    TrieNode('(', TokenType::LeftParen, true),                 // 8
    TrieNode(')', TokenType::RightParen, true),                // 9
    TrieNode('*', 11, 1, TokenType::Star, true, true),         // 10
    TrieNode('=', TokenType::StarEqual),                       // 11
    TrieNode('+', 13, 2, TokenType::Plus, true, true),         // 12
    TrieNode('+', TokenType::DoublePlus),                      // 13
    TrieNode('=', TokenType::PlusEqual),                       // 14
    TrieNode(',', TokenType::Comma, true),                     // 15
    TrieNode('-', 17, 2, TokenType::Minus, true, true),        // 16
    TrieNode('-', TokenType::DoubleMinus),                     // 17
    TrieNode('=', TokenType::MinusEqual),                      // 18
    TrieNode('.', TokenType::Dot, true),                       // 19
    TrieNode('/', 21, 1, TokenType::ForwardSlash, true, true), // 20
    TrieNode('=', TokenType::SlashEqual),                      // 21
    TrieNode(':', TokenType::Colon, true),                     // 22
    TrieNode(';', TokenType::Semicolon, true),                 // 23
    TrieNode('<', 25, 2, TokenType::LeftAngle, true, true),    // 24
    TrieNode('<', 26, 1, TokenType::LeftShift, true, true),    // 25
    TrieNode('=', TokenType::LeftShiftEquals),                 // 26
    TrieNode('=', TokenType::SmallerEqual),                    // 27
    TrieNode('=', 29, 2, TokenType::Equal, true, true),        // 28
    TrieNode('=', TokenType::DoubleEqual),                     // 29
    TrieNode('>', TokenType::FuncArrow),                       // 30
    TrieNode('>', 32, 2, TokenType::RightAngle, true, true),   // 31
    TrieNode('=', TokenType::BiggerEqual),                     // 32
    TrieNode('>', 34, 2, TokenType::RightShift, true, true),   // 33
    TrieNode('=', TokenType::RightShiftEquals),                // 34
    TrieNode('>', 36, 1, TokenType::TripleShift, true, true),  // 35
    TrieNode('=', TokenType::TripleShiftEquals),               // 36
    TrieNode('@', TokenType::At, true),                        // 37
    TrieNode('[', TokenType::LeftSquare, true),                // 38
    TrieNode(']', TokenType::RightSquare, true),               // 39
    TrieNode('^', 41, 1, TokenType::Carrot, true, true),       // 40
    TrieNode('=', TokenType::CarrotEquals),                    // 41
    TrieNode('a', 43, 2, true),                                // 42
    TrieNode('n', 44, 1),                                      // 43
    TrieNode('d', TokenType::And),                             // 44
    TrieNode('s', 46, 1),                                      // 45
    TrieNode('m', TokenType::Asm),                             // 46
    TrieNode('b', 48, 2, true),                                // 47
    TrieNode('o', 49, 1),                                      // 48
    TrieNode('o', 50, 1),                                      // 49
    TrieNode('l', TokenType::Bool),                            // 50
    TrieNode('r', 52, 1),                                      // 51
    TrieNode('a', 53, 1),                                      // 52
    TrieNode('n', 54, 1),                                      // 53
    TrieNode('c', 55, 1),                                      // 54
    TrieNode('h', TokenType::Branch),                          // 55
    TrieNode('c', 57, 1, true),                                // 56
    TrieNode('h', 58, 1),                                      // 57
    TrieNode('a', 59, 1),                                      // 58
    TrieNode('r', TokenType::Char),                            // 59
    TrieNode('e', 61, 2, true),                                // 60
    TrieNode('l', 62, 2),                                      // 61
    TrieNode('i', 63, 1),                                      // 62
    TrieNode('f', TokenType::Elif),                            // 63
    TrieNode('s', 65, 1),                                      // 64
    TrieNode('e', TokenType::Else),                            // 65
    TrieNode('x', 67, 1),                                      // 66
    TrieNode('p', 68, 1),                                      // 67
    TrieNode('o', 69, 1),                                      // 68
    TrieNode('r', 70, 1),                                      // 69
    TrieNode('t', TokenType::Export),                          // 70
    TrieNode('f', 72, 3, true),                                // 71
    TrieNode('a', 73, 1),                                      // 72
    TrieNode('l', 74, 1),                                      // 73
    TrieNode('s', 75, 1),                                      // 74
    TrieNode('e', TokenType::False),                           // 75
    TrieNode('l', 77, 1),                                      // 76
    TrieNode('o', 78, 1),                                      // 77
    TrieNode('a', 79, 1),                                      // 78
    TrieNode('t', TokenType::Float),                           // 79
    TrieNode('u', 81, 1),                                      // 80
    TrieNode('n', 82, 1),                                      // 81
    TrieNode('c', 83, 1),                                      // 82
    TrieNode('t', 84, 1),                                      // 83
    TrieNode('i', 85, 1),                                      // 84
    TrieNode('o', 86, 1),                                      // 85
    TrieNode('n', TokenType::Function),                        // 86
    TrieNode('i', 88, 3, true),                                // 87
    TrieNode('f', TokenType::If),                              // 88
    TrieNode('m', 90, 1),                                      // 89
    TrieNode('p', 91, 1),                                      // 90
    TrieNode('o', 92, 1),                                      // 91
    TrieNode('r', 93, 1),                                      // 92
    TrieNode('t', TokenType::Import),                          // 93
    TrieNode('n', 95, 1, TokenType::In, true, true),           // 94
    TrieNode('t', TokenType::Int),                             // 95
    TrieNode('l', 97, 2, true),                                // 96
    TrieNode('e', 98, 1),                                      // 97
    TrieNode('t', TokenType::Let),                             // 98
    TrieNode('o', 100, 1),                                     // 99
    TrieNode('o', 101, 1),                                     // 100
    TrieNode('p', TokenType::Loop),                            // 101
    TrieNode('m', 103, 1, true),                               // 102
    TrieNode('o', 104, 1),                                     // 103
    TrieNode('d', 105, 1),                                     // 104
    TrieNode('u', 106, 1),                                     // 105
    TrieNode('l', 107, 1),                                     // 106
    TrieNode('e', TokenType::Module),                          // 107
    TrieNode('n', 109, 1, true),                               // 108
    TrieNode('u', 110, 1),                                     // 109
    TrieNode('l', 111, 1),                                     // 110
    TrieNode('l', TokenType::Null),                            // 111
    TrieNode('o', 113, 1, true),                               // 112
    TrieNode('r', TokenType::Or),                              // 113
    TrieNode('p', 115, 1, true),                               // 114
    TrieNode('e', 116, 1),                                     // 115
    TrieNode('r', 117, 1),                                     // 116
    TrieNode('s', 118, 1),                                     // 117
    TrieNode('i', 119, 1),                                     // 118
    TrieNode('s', 120, 1),                                     // 119
    TrieNode('t', TokenType::Persist),                         // 120
    TrieNode('r', 122, 1, true),                               // 121
    TrieNode('e', 123, 1),                                     // 122
    TrieNode('t', 124, 1),                                     // 123
    TrieNode('u', 125, 1),                                     // 124
    TrieNode('r', 126, 1),                                     // 125
    TrieNode('n', TokenType::Return),                          // 126
    TrieNode('s', 128, 1, true),                               // 127
    TrieNode('p', 129, 1),                                     // 128
    TrieNode('e', 130, 1),                                     // 129
    TrieNode('c', TokenType::Spec),                            // 130
    TrieNode('t', 132, 3, true),                               // 131
    TrieNode('e', 133, 1),                                     // 132
    TrieNode('m', 134, 1),                                     // 133
    TrieNode('p', 135, 1),                                     // 134
    TrieNode('l', 136, 1),                                     // 135
    TrieNode('a', 137, 1),                                     // 136
    TrieNode('t', 138, 1),                                     // 137
    TrieNode('e', TokenType::Template),                        // 138
    TrieNode('r', 140, 1),                                     // 139
    TrieNode('u', 141, 1),                                     // 140
    TrieNode('e', TokenType::True),                            // 141
    TrieNode('y', 143, 1),                                     // 142
    TrieNode('p', 144, 1),                                     // 143
    TrieNode('e', 145, 1, TokenType::Type, true, true),        // 144
    TrieNode('o', 146, 1),                                     // 145
    TrieNode('f', TokenType::Typeof),                          // 146
    TrieNode('u', 148, 1, true),                               // 147
    TrieNode('i', 149, 1),                                     // 148
    TrieNode('n', 150, 1),                                     // 149
    TrieNode('t', TokenType::Uint),                            // 150
    TrieNode('w', 152, 1, true),                               // 151
    TrieNode('h', 153, 1),                                     // 152
    TrieNode('e', 154, 1),                                     // 153
    TrieNode('n', TokenType::When),                            // 154
    TrieNode('{', TokenType::LeftCurly, true),                 // 155
    TrieNode('|', 157, 1, TokenType::Pipe, true, true),        // 156
    TrieNode('=', TokenType::PipeEquals),                      // 157
    TrieNode('}', TokenType::RightCurly, true),                // 158
    TrieNode('~', TokenType::Tilda, true),                     // 159
};

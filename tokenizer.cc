#include "tokenizer.h"
#include <sstream>
#include <iostream>
#include <cctype>

/* std::vector<Token> tokenize(const std::string &input)
{
    std::vector<Token> tokens;
    std::istringstream stream(input);
    std::string word;

    while (stream >> word)
    {
        Token token;

        if (word == "|")
            token.type = TOKEN_PIPE;
        else if (word == ">")
            token.type = TOKEN_REDIRECT;
        else if (word == ">>")
            token.type = TOKEN_APPEND;
        else if (word == "<")
            token.type = TOKEN_INPUT;
        else if (word == "&")
            token.type = TOKEN_BACKGROUND;
        else if (word == "2>")
            token.type = TOKEN_ERROR;
        else if (word == ">>&")
            token.type = TOKEN_REDIRECT_AND_ERROR;
        else
            {
                if (!tokens.empty())
                {
                    Token &prev = tokens.back();
                    if (prev.type == TOKEN_REDIRECT ||
                        prev.type == TOKEN_APPEND ||
                        prev.type == TOKEN_INPUT ||
                        prev.type == TOKEN_ERROR || 
                        prev.type == TOKEN_REDIRECT_AND_ERROR ||
                        prev.type == TOKEN_COMMAND || 
                        prev.type == TOKEN_ARGUMENT || 
                        word[0] == '-' ||
                        word.find('/') != std::string::npos ||
                        word.find('.') != std::string::npos

                    )
                    {
                        token.type = TOKEN_ARGUMENT;
                    }
                    else
                    {
                        token.type = TOKEN_COMMAND;
                    }
                }
                else
                {
                    token.type = TOKEN_COMMAND;
                }
            }

        token.value = word;
        tokens.push_back(token);
    }

    tokens.push_back({TOKEN_EOF, ""});
    return tokens;
}
*/

std::vector<Token> tokenize(const std::string &input)
{
    std::vector<Token> tokens;
    std::string word;

    bool expectCommand = true;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool wordStarted = false;

    auto addWord = [&]()
    {
        if (!wordStarted)
            return;

        Token token;

        if (expectCommand)
        {
            token.type = TOKEN_COMMAND;
            expectCommand = false;
        }
        else
        {
            token.type = TOKEN_ARGUMENT;
        }

        token.value = word;
        tokens.push_back(token);
        word.clear();
        wordStarted = false;
    };

    for (size_t i = 0; i < input.size(); i++)
    {
        char c = input[i];
        if (inSingleQuote)
        {
            if (c == '\'')
            {
                inSingleQuote = false;
            }
            else
            {
                word += c;
            }

            continue;
        }
        if (inDoubleQuote)
        {
            if (c == '"')
            {
                inDoubleQuote = false;
            }
            else
            {
                word += c;
            }

            continue;
        }
        if (c == '\'')
        {
            inSingleQuote = true;
            wordStarted = true;
            continue;
        }
        if (c == '"')
        {
            inDoubleQuote = true;
            wordStarted = true;
            continue;
        }
        if(c =='2' && !wordStarted && i+1 < input.size() && input[i+1] == '>'){
            tokens.push_back({TOKEN_ERROR, "2>"});
            i++;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c)))
        {
            addWord();
            continue;
        }

        if (c == '|')
        {
            addWord();
            tokens.push_back({TOKEN_PIPE, "|"});
            expectCommand = true;
            continue;
        }

        if (c == '<')
        {
            addWord();
            tokens.push_back({TOKEN_INPUT, "<"});
            continue;
        }

        if (c == '>')
        {
            addWord();
            if(i+2 < input.size() && input[i + 1] == '>' && input[i + 2] == '&'){
                tokens.push_back({TOKEN_REDIRECT_AND_ERROR, ">>&"});
                i += 2;
            }
            else if (i + 1 < input.size() && input[i + 1] == '>')
            {
                tokens.push_back({TOKEN_APPEND, ">>"});
                i++;
            }
            else
            {
                tokens.push_back({TOKEN_REDIRECT, ">"});
            }

            continue;
        }

        if (c == '&')
        {
            addWord();
            tokens.push_back({TOKEN_BACKGROUND, "&"});
            continue;
        }

        word += c;
        wordStarted =true;
    }

    if(inSingleQuote || inDoubleQuote){
        std::cerr << "error : unclosed quote" << std::endl;
        return {{TOKEN_EOF,""}};
    }

    addWord();

    tokens.push_back({TOKEN_EOF, ""});

    return tokens;
}
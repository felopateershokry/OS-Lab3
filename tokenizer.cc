#include "tokenizer.h"
#include <sstream>
#include <iostream>
#include <cctype>

std::vector<Token> tokenize(const std::string &input){
    std::vector<Token> tokens;
    std::string word;

    bool expectCommand = true;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool wordStarted = false;

    auto addWord = [&](){
        if(!wordStarted)
            return;
        Token token;
        if(expectCommand){
            token.type = TOKEN_COMMAND;
            expectCommand = false;
        }else{
            token.type = TOKEN_ARGUMENT;
        }

        token.value = word;
        tokens.push_back(token);
        word.clear();
        wordStarted = false;
    };

    for(size_t i = 0; i < input.size(); i++){
        char c = input[i];
        if(inSingleQuote){
            if(c == '\''){
                inSingleQuote = false;
            }else{
                word += c;
            }
            continue;
        }

        if(inDoubleQuote){
            if(c == '"'){
                inDoubleQuote = false;
            }else{
                word += c;
            }
            continue;
        }
        if(c == '\''){
            inSingleQuote = true;
            wordStarted = true;
            continue;
        }
        if(c == '"'){
            inDoubleQuote = true;
            wordStarted = true;
            continue;
        }
        if(c =='2' && !wordStarted && i+1 < input.size() && input[i+1] == '>'){
            tokens.push_back({TOKEN_ERROR, "2>"});
            i++;
            continue;
        }

        if(std::isspace(static_cast<unsigned char>(c))){
            addWord();
            continue;
        }

        if(c == '|'){
            addWord();
            tokens.push_back({TOKEN_PIPE, "|"});
            expectCommand = true;
            continue;
        }

        if(c == '<'){
            addWord();
            tokens.push_back({TOKEN_INPUT, "<"});
            continue;
        }

        if(c == '>'){
            addWord();
            if(i+2 < input.size() && input[i + 1] == '>' && input[i + 2] == '&'){
                tokens.push_back({TOKEN_REDIRECT_AND_ERROR, ">>&"});
                i += 2;
            }else if(i + 1 < input.size() && input[i + 1] == '>'){
                tokens.push_back({TOKEN_APPEND, ">>"});
                i++;
            }else{
                tokens.push_back({TOKEN_REDIRECT, ">"});
            }

            continue;
        }

        if(c == '&'){
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
#ifndef CONFIG_TOKENIZER_HPP
# define CONFIG_TOKENIZER_HPP

# include <cstddef>
# include <string>
# include <vector>
# include "config/ConfigToken.hpp"

class ConfigTokenizer
{
    public:
        ConfigTokenizer();
        ConfigTokenizer(const ConfigTokenizer &other);
        ConfigTokenizer&    operator=(const ConfigTokenizer &other);
        ~ConfigTokenizer();

        std::vector<ConfigToken>    tokenize(const std::string &content);
    private:
        std::string     _content;
        std::size_t     _index;
        unsigned int    _line;
        unsigned int    _column;

        bool            isAtEnd(void) const;
        char            peek() const;
        char            advance();
        void            skipWhitespaceAndComments();
        ConfigToken     readWord();
        ConfigToken     makeToken(ConfigTokenType type,
            const std::string &value) const;
};

#endif
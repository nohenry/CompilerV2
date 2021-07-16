#pragma once

#include <string>

class Colors
{
private:
    friend class color;

    std::string str;
    Colors(std::string str) : str{str}
    {
    }

public:
    const static bool enabled = true;

    /*
    *   Reset
    */
    Colors &reset()
    {
        if (enabled)
        {
            str = stylize(0, 0);
        }
        return *this;
    }

    /* #region  Styles */
    Colors &bold()
    {
        if (enabled)
        {
            str = stylize(1, 22);
        }
        return *this;
    }

    Colors &dim()
    {
        if (enabled)
        {
            str = stylize(2, 22);
        }
        return *this;
    }

    Colors &italic()
    {
        if (enabled)
        {
            str = stylize(3, 23);
        }
        return *this;
    }

    Colors &underline()
    {
        if (enabled)
        {
            str = stylize(4, 24);
        }
        return *this;
    }

    Colors &inverse()
    {
        if (enabled)
        {
            str = stylize(7, 27);
        }
        return *this;
    }

    Colors &hidden()
    {
        if (enabled)
        {
            str = stylize(8, 28);
        }
        return *this;
    }

    Colors &strikethrough()
    {
        if (enabled)
        {
            str = stylize(9, 29);
        }
        return *this;
    }
    /* #endregion */

    /* #region  Colors */
    Colors &black()
    {
        if (enabled)
        {
            str = stylize(30, 39);
        }
        return *this;
    }

    Colors &red()
    {
        if (enabled)
        {
            str = stylize(31, 39);
        }
        return *this;
    }

    Colors &green()
    {
        if (enabled)
        {
            str = stylize(32, 39);
        }
        return *this;
    }

    Colors &yellow()
    {
        if (enabled)
        {
            str = stylize(33, 39);
        }
        return *this;
    }

    Colors &blue()
    {
        if (enabled)
        {
            str = stylize(34, 39);
        }
        return *this;
    }

    Colors &magenta()
    {
        if (enabled)
        {
            str = stylize(35, 39);
        }
        return *this;
    }

    Colors &cyan()
    {
        if (enabled)
        {
            str = stylize(36, 39);
        }
        return *this;
    }

    Colors &white()
    {
        if (enabled)
        {
            str = stylize(37, 39);
        }
        return *this;
    }

    Colors &grey()
    {
        if (enabled)
        {
            str = stylize(90, 39);
        }
        return *this;
    }

    Colors &gray()
    {
        if (enabled)
        {
            str = stylize(90, 39);
        }
        return *this;
    }
    /* #endregion */

    /* #region  Bright Colors */
    Colors &brightRed()
    {
        if (enabled)
        {
            str = stylize(91, 39);
        }
        return *this;
    }

    Colors &brightGreen()
    {
        if (enabled)
        {
            str = stylize(92, 39);
        }
        return *this;
    }

    Colors &brightYellow()
    {
        if (enabled)
        {
            str = stylize(93, 39);
        }
        return *this;
    }

    Colors &brightBlue()
    {
        if (enabled)
        {
            str = stylize(94, 39);
        }
        return *this;
    }

    Colors &brightMagenta()
    {
        if (enabled)
        {
            str = stylize(95, 39);
        }
        return *this;
    }

    Colors &brightCyan()
    {
        if (enabled)
        {
            str = stylize(96, 39);
        }
        return *this;
    }

    Colors &brightWhite()
    {
        if (enabled)
        {
            str = stylize(97, 39);
        }
        return *this;
    }
    /* #endregion */

    /* #region  Background Colors */
    Colors &bgBlack()
    {
        if (enabled)
        {
            str = stylize(40, 49);
        }
        return *this;
    }

    Colors &bgRed()
    {
        if (enabled)
        {
            str = stylize(41, 49);
        }
        return *this;
    }

    Colors &bgGreen()
    {
        if (enabled)
        {
            str = stylize(42, 49);
        }
        return *this;
    }

    Colors &bgYellow()
    {
        if (enabled)
        {
            str = stylize(43, 49);
        }
        return *this;
    }

    Colors &bgBlue()
    {
        if (enabled)
        {
            str = stylize(44, 49);
        }
        return *this;
    }

    Colors &bgMagenta()
    {
        if (enabled)
        {
            str = stylize(45, 49);
        }
        return *this;
    }

    Colors &bgCyan()
    {
        if (enabled)
        {
            str = stylize(46, 49);
        }
        return *this;
    }

    Colors &bgWhite()
    {
        if (enabled)
        {
            str = stylize(47, 49);
        }
        return *this;
    }

    Colors &bgGrey()
    {
        if (enabled)
        {
            str = stylize(100, 49);
        }
        return *this;
    }

    Colors &bgGray()
    {
        if (enabled)
        {
            str = stylize(100, 49);
        }
        return *this;
    }
    /* #endregion */

    /* #region Bright Background Colors */
    Colors &bgBrightRed()
    {
        if (enabled)
        {
            str = stylize(101, 49);
        }
        return *this;
    }

    Colors &bgBrightGreen()
    {
        if (enabled)
        {
            str = stylize(102, 49);
        }
        return *this;
    }

    Colors &bgBrightYellow()
    {
        if (enabled)
        {
            str = stylize(103, 49);
        }
        return *this;
    }

    Colors &bgBrightBlue()
    {
        if (enabled)
        {
            str = stylize(104, 49);
        }
        return *this;
    }

    Colors &bgBrightMagenta()
    {
        if (enabled)
        {
            str = stylize(105, 49);
        }
        return *this;
    }

    Colors &bgBrightCyan()
    {
        if (enabled)
        {
            str = stylize(106, 49);
        }
        return *this;
    }

    Colors &bgBrightWhite()
    {
        if (enabled)
        {
            str = stylize(107, 49);
        }
        return *this;
    }
    /* #endregion */

    operator std::string()
    {
        return str;
    }

    std::string stylize(uint32_t open, uint32_t close)
    {
        return "\x1b[" + std::to_string(open) + "m" + str + "\x1b[" + std::to_string(close) + "m";
    }

    static std::string stylize(uint32_t val)
    {
        return "\x1b[" + std::to_string(val) + "m" ;
    }

    const std::string &value() const
    {
        return str;
    }
};

class color
{
public:
    static Colors reset(std::string str)
    {
        return Colors(str).reset();
    }

    static Colors bold(std::string str)
    {
        return Colors(str).bold();
    }

    static Colors dim(std::string str)
    {
        return Colors(str).dim();
    }

    static Colors italic(std::string str)
    {
        return Colors(str).italic();
    }

    static Colors underline(std::string str)
    {
        return Colors(str).underline();
    }

    static Colors inverse(std::string str)
    {
        return Colors(str).inverse();
    }

    static Colors hidden(std::string str)
    {
        return Colors(str).hidden();
    }

    static Colors strikethrough(std::string str)
    {
        return Colors(str).strikethrough();
    }

    static Colors black(std::string str)
    {
        return Colors(str).black();
    }

    static Colors red(std::string str)
    {
        return Colors(str).red();
    }

    static Colors green(std::string str)
    {
        return Colors(str).green();
    }

    static Colors yellow(std::string str)
    {
        return Colors(str).yellow();
    }

    static Colors blue(std::string str)
    {
        return Colors(str).blue();
    }

    static Colors magenta(std::string str)
    {
        return Colors(str).magenta();
    }

    static Colors cyan(std::string str)
    {
        return Colors(str).cyan();
    }

    static Colors white(std::string str)
    {
        return Colors(str).white();
    }

    static Colors grey(std::string str)
    {
        return Colors(str).grey();
    }

    static Colors gray(std::string str)
    {
        return Colors(str).gray();
    }

    static Colors brightRed(std::string str)
    {
        return Colors(str).red();
    }

    static Colors brightGreen(std::string str)
    {
        return Colors(str).brightGreen();
    }

    static Colors brightYellow(std::string str)
    {
        return Colors(str).brightYellow();
    }

    static Colors brightBlue(std::string str)
    {
        return Colors(str).brightBlue();
    }

    static Colors brightMagenta(std::string str)
    {
        return Colors(str).brightMagenta();
    }

    static Colors brightCyan(std::string str)
    {
        return Colors(str).brightCyan();
    }

    static Colors brightWhite(std::string str)
    {
        return Colors(str).brightWhite();
    }

    static Colors bgBlack(std::string str)
    {
        return Colors(str).bgBlack();
    }

    static Colors bgRed(std::string str)
    {
        return Colors(str).bgRed();
    }

    static Colors bgGreen(std::string str)
    {
        return Colors(str).bgGreen();
    }

    static Colors bgYellow(std::string str)
    {
        return Colors(str).bgYellow();
    }

    static Colors bgBlue(std::string str)
    {
        return Colors(str).bgBlue();
    }

    static Colors bgMagenta(std::string str)
    {
        return Colors(str).bgMagenta();
    }

    static Colors bgCyan(std::string str)
    {
        return Colors(str).bgCyan();
    }

    static Colors bgWhite(std::string str)
    {
        return Colors(str).bgWhite();
    }

    static Colors bgGrey(std::string str)
    {
        return Colors(str).bgGrey();
    }

    static Colors bgGray(std::string str)
    {
        return Colors(str).bgGray();
    }

    static Colors bgBrightRed(std::string str)
    {
        return Colors(str).bgBrightRed();
    }

    static Colors bgBrightGreen(std::string str)
    {
        return Colors(str).bgBrightGreen();
    }

    static Colors bgBrightYellow(std::string str)
    {
        return Colors(str).bgBrightYellow();
    }

    static Colors bgBrightBlue(std::string str)
    {
        return Colors(str).bgBrightBlue();
    }

    static Colors bgBrightMagenta(std::string str)
    {
        return Colors(str).bgBrightMagenta();
    }

    static Colors bgBrightCyan(std::string str)
    {
        return Colors(str).bgBrightCyan();
    }

    static Colors bgBrightWhite(std::string str)
    {
        return Colors(str).bgBrightWhite();
    }
};

std::ostream &operator<<(std::ostream &stream, const Colors &colors);
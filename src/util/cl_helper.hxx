#ifndef __cl_helper_hxx_
#define __cl_helper_hxx_

#include <cerrno>
#include <sstream>
#include "format_exception.hxx"
#include "itksys/SystemTools.hxx"

/**
 * @brief A class to facilitate processing of command-line arguments
 */
class CommandLineHelper
{
public:
  CommandLineHelper(int argc, char *argv[], int first_arg = 1)
  {
    this->argc = argc;
    this->argv = argv;
    i = first_arg;
  }

  /**
   * Set the data root directory. All filenames will be read relative to this
   */
  void set_data_root(const char *root = nullptr)
  {
    if(root)
      data_root = root;
    else
      data_root = std::string();
  }

  bool is_at_end()
  {
    return i >= argc;
  }

  /**
   * Just read the next arg (used internally)
   */
  const char *read_arg()
  {
    if(i >= argc)
      throw FormatException("Unexpected end of command line arguments.");

    return argv[i++];
  }

  /**
   * Peek at the next argument, without treating it as 'read'. Useful when arguments
   * of different type are possible
   */
  const char *peek_arg()
  {
    if(i >= argc)
      throw FormatException("Unexpected end of command line arguments.");
    return argv[i];
  }

  /**
   * Read a command (something that starts with a '-')
   */
  std::string read_command()
  {
    current_command = read_arg();
    if(current_command[0] != '-')
      throw FormatException("Expected a command at position %d, instead got '%s'.", i, current_command.c_str());
    return current_command;
  }

  /**
   * Read a command if available (there are arguments remaining)
   */
  bool read_command(std::string &cmd)
  {
    if(i >= argc || peek_arg()[0] != '-')
      return false;
    cmd = read_command();
    return true;
  }

  /**
   * Read a string that is not a command (may not start with a -)
   */
  std::string read_string()
  {
    std::string arg = read_arg();
    if(arg[0] == '-')
      throw FormatException("Expected a string argument as parameter to '%s', instead got '%s'.", current_command.c_str(), arg.c_str());

    return arg;
  }


  /**
   * Get the number of free arguments to the current command. Use only for commands with
   * a priori unknown number of arguments. Otherwise, just use the get_ commands
   */
  int command_arg_count(int min_required = 0)
  {
    // Count the number of arguments
    int n_args = 0;
    for(int j = i; j < argc; j++, n_args++)
      if(argv[j][0] == '-')
        break;

    // Test for minimum required
    if(n_args < min_required)
      throw FormatException(
          "Expected at least %d arguments to '%s', instead got '%d'",
          min_required, current_command.c_str(), n_args);

    return n_args;
  }

  /**
   * Read an existing filename
   */
  std::string read_existing_filename()
  {
    std::string file = read_arg();
    if(this->data_root.length())
      file = itksys::SystemTools::CollapseFullPath(file, data_root);

    if(!itksys::SystemTools::FileExists(file.c_str()))
      throw FormatException("File '%s' does not exist", file.c_str());

    return file;
  }

  /**
   * Read an output filename
   */
  std::string read_output_filename()
  {
    std::string file = read_arg();
    if(this->data_root.length())
      file = itksys::SystemTools::CollapseFullPath(file, data_root);
    return file;
  }

  /**
   * Read a floating point value
   */
  double read_double()
  {
    std::string arg = read_arg();

    errno = 0; char *pend;
    double val = std::strtod(arg.c_str(), &pend);

    if(errno || *pend)
      throw FormatException("Expected a floating point number as parameter to '%s', instead got '%s'",
                            current_command.c_str(), arg.c_str());

    return val;
  }

  /**
   * Read output directory path
   */
  std::string read_output_dir()
  {
    std::string dir = read_arg();
    if(this->data_root.length())
      dir = itksys::SystemTools::CollapseFullPath(dir, data_root);

    if(!itksys::SystemTools::PathExists(dir.c_str()))
      throw FormatException("Folder '%s' does not exist", dir.c_str());

    return dir;
  }

  /**
   * Check if a string ends with another string and return the
   * substring without the suffix
   */
  bool check_suffix(const std::string &source, const std::string &suffix, std::string &out_prefix)
  {
    int n = source.length(), m = suffix.length();
    if(n < m)
      return false;

    if(source.substr(n-m, m) != suffix)
      return false;

    out_prefix = source.substr(0, n-m);
    return true;
  }

  /**
   * Read a floating point value with units (mm or vox)
   */
  double read_scalar_with_units(bool &physical_units)
  {
    std::string arg = read_arg();
    std::string scalar;

    if(check_suffix(arg, "vox", scalar))
      physical_units = false;
    else if(check_suffix(arg, "mm", scalar))
      physical_units = true;
    else
      throw FormatException("Parameter to '%s' should include units, e.g. '3vox' or '3mm', instead got '%s'",
                            current_command.c_str(), arg.c_str());

    errno = 0; char *pend;
    double val = std::strtod(scalar.c_str(), &pend);

    if(errno || *pend)
      throw FormatException("Expected a floating point number as parameter to '%s', instead got '%s'",
                            current_command.c_str(), scalar.c_str());

    return val;
  }

  /**
   * Read an integer value
   */
  long read_integer()
  {
    std::string arg = read_arg();

    errno = 0; char *pend;
    long val = std::strtol(arg.c_str(), &pend, 10);

    if(errno || *pend)
      throw FormatException("Expected an integer as parameter to '%s', instead got '%s'",
                            current_command.c_str(), arg.c_str());

    return val;
  }

  /**
   * Read one of a list of strings. The optional parameters to this are in the form
   * int, string, int, string, int, string. Each string may in turn contain a list
   * of words (separated by space) that are acceptable. So for example. NULL string
   * is used to refer to the default option.
   *
   * enum Mode { NORMAL, BAD, SILLY }
   * Mode m = X.read_option(NORMAL, "NORMAL normal", BAD, "bad BAD", SILLY, NULL);
   */
  /*
  template <class TOption>
  TOption read_option(TOption opt1, const char *str1, ...)
  {
    not implemented yet
  }
  */

  /**
   * Read a vector in the format 1.0x0.2x0.6
   */
  std::vector<double> read_double_vector()
  {
    std::string arg = read_arg();
    std::istringstream f(arg);
    std::string s;
    std::vector<double> vector;
    while (getline(f, s, 'x'))
      {
      errno = 0; char *pend;
      double val = std::strtod(s.c_str(), &pend);

      if(errno || *pend)
        throw FormatException("Expected a floating point vector as parameter to '%s', instead got '%s'",
                              current_command.c_str(), arg.c_str());
      vector.push_back(val);
      }

    if(!vector.size())
      throw FormatException("Expected a floating point vector as parameter to '%s', instead got '%s'",
                            current_command.c_str(), arg.c_str());

    return vector;
  }

  std::vector<int> read_int_vector(char delimiter = 'x')
  {
    std::string arg = read_arg();
    std::istringstream f(arg);
    std::string s;
    std::vector<int> vector;
    while (getline(f, s, delimiter))
      {
      errno = 0; char *pend;
      long val = std::strtol(s.c_str(), &pend, 10);

      if(errno || *pend)
        throw FormatException("Expected an integer vector delimited by '%c' as parameter to '%s', instead got '%s'",
                              delimiter, current_command.c_str(), arg.c_str());
      vector.push_back((int) val);
      }

    if(!vector.size())
      throw FormatException("Expected an integer vector delimited by '%c' as parameter to '%s', instead got '%s'",
                            delimiter, current_command.c_str(), arg.c_str());
    return vector;
  }

  /** Remove the last k commands from the parser, and return a new parser for those commands */
  CommandLineHelper take_end(int k)
  {
    if(k > argc)
      throw FormatException("Cannot take last %d commands from commandline", k);

    argc -= k;
    return CommandLineHelper(k, argv + argc, 0);
  }





private:
  int argc, i;
  char **argv;
  std::string current_command;
  std::string data_root;
};

#endif // __cl_helper_hxx_

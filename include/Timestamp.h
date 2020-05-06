/* encoding=utf-8*/
#pragma once

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>

// micro-second (µs) based
constexpr unsigned long TIMESTAMP_TO_SECONDS_DIVISOR = 1'000'000UL;

// string is always 6.6, no matter what the above divisor is.
// "123456.654321" microseconds, "######.######",  i.e. regex: \d{6}\.\d{6}
constexpr int TIMESTAMP_STRING_DECIMAL = 6;
constexpr int TIMESTAMP_STRING_LEN =
    TIMESTAMP_STRING_DECIMAL + 1 + TIMESTAMP_STRING_DECIMAL; // 13 characters

template <typename REP = int64_t,
          typename PERIOD = std::ratio<1, TIMESTAMP_TO_SECONDS_DIVISOR>>
class CTimestampBase {
public:
  using Duration = std::chrono::duration<REP, PERIOD>;
  using Rep = REP;
  using Period = PERIOD;
  constexpr static auto numeric_limits_max = std::numeric_limits<REP>::max();

private:
  Duration dd; // Event timestamp in _Period, default to micro-seconds,
               // represented in int64_t
               // REP dd;

public: // todo: dicision on negtive value?
  constexpr CTimestampBase() = default;
  constexpr CTimestampBase(const CTimestampBase &src) = default;
  explicit constexpr CTimestampBase(int i) : dd(i){};

private:
  CTimestampBase(double i);
  // explicit constexpr CTimestampBase(double i)
  //    : dd(static_cast<REP>(i * 1000000)) {};
  //// template <typename  T>
  // CTimestampBase(T i);

  // template <class T, typename std::enable_if<std::is_integral<T>::value,
  // int>::type* = nullptr> CTimestampBase(T i);

  //~CTimestampBase() = default;

  // constexpr REP Count() const { return dd.count(); }

  /** Cast operator.  */
  // operator _Rep() const { return dd.count(); } //??? dangrous!
  // todo: decision on casting to _Rep?

  CTimestampBase &
  operator+=(const CTimestampBase &ts) { // overflow when a+b>max; i.e. a>max-b
    bool bOverflow = (dd > numeric_limits_max - ts.dd);
    if (bOverflow) {
      std::stringstream strm;
      strm << "Timestamp overflow: " << __FILE__ << " (Line " << __LINE__
           << ")";
      throw std::out_of_range(strm.str());
    }

    dd += ts.dd;
    return *this;
  }

  /** -= operator.
   * @return A reference to this timestamp object.
   */
  constexpr CTimestampBase &operator-=(const CTimestampBase ts) {
    if (ts.dd > dd)
      dd = Duration{0}; // todo: who needs this? when small minus big?
    else
      dd -= ts.dd;
    return *this;
  }

  /** Convert the timestamp to a string.
   * @param ts The timestamp in ratio<1, TIMESTAMP_TO_SECONDS_DIVISOR>.
   * @return The timestamp formatted as a string.
   * @note //when exceed 6.6 bits, keep as it is.*/
  static std::string
  ToString(REP const ts) { // "123456.654321" microseconds, "######.######",
                           // i.e. regex: \d{6}\.\d{6}
    if (ts > 0) {
      auto _seconds = std::chrono::seconds{ts / TIMESTAMP_TO_SECONDS_DIVISOR};
      // std::chrono::microseconds _fraction = Duration{ts %
      // TIMESTAMP_TO_SECONDS_DIVISOR};
      std::chrono::microseconds _fraction = Duration{ts} - _seconds;

      std::stringstream retval;
      retval << std::dec << std::setw(TIMESTAMP_STRING_DECIMAL)
             << std::setfill('0') << _seconds.count() << "."
             << std::setw(TIMESTAMP_STRING_DECIMAL) << std::setfill('0')
             << _fraction.count();
      return retval.str();
    } else if (ts == 0) {
      return std::string("000000.000000");
    } else // ts <= 0
    {
      /* double lose pricision near min() double seconds =
       * CTimestampBase{ts}.ToSeconds();*/
      // std::stringstream retval;
      // retval << std::fixed << std::internal << std::setw(TIMESTAMP_STRING_LEN
      // + 1)
      //       << std::setprecision(TIMESTAMP_STRING_DECIMAL) <<
      //       std::setfill('0') << seconds;
      // return retval.str();

      auto _seconds =
          std::chrono::seconds{-(ts / TIMESTAMP_TO_SECONDS_DIVISOR)};
      // std::chrono::microseconds _fraction = Duration{-1 * (ts %
      // TIMESTAMP_TO_SECONDS_DIVISOR)};
      std::chrono::microseconds _fraction = -(Duration{ts} + _seconds);

      std::stringstream retval;
      retval << "-" << std::dec << std::setw(TIMESTAMP_STRING_DECIMAL)
             << std::setfill('0') << _seconds.count() << "."
             << std::setw(TIMESTAMP_STRING_DECIMAL) << std::setfill('0')
             << _fraction.count();
      return retval.str();
    }
  }

  /** Convert the timestamp to a string.
   * @return The timestamp formatted as a string.
   * @note //when exceed 6.6 bits, keep as it is.     */
  std::string ToString() const { // "123456.654321" microseconds,
                                 // "######.######",  i.e. regex: \d{6}\.\d{6}
    return ToString(dd.count());
  }

  /** Set the timestamp from a string.
   * @param sTimeString The timestamp formatted as a string in seconds.
   * @param bValidateFormat Validate that the format of the timestamp string is
   * ######.######.
   * @return A reference to this timestamp object.
   */
  CTimestampBase &
  FromString(const std::string &sTimeString,
             bool const bValidateFormat =
                 false) { // "123456.654321" microseconds, "######.######", i.e.
                          // regex: \d{6}\.\d{6}
    if (bValidateFormat) { // Assumed format: "######.######"
      const std::regex myRegexObj{R"(\d{6}\.\d{6})"};
      const auto bValid = regex_match(sTimeString, myRegexObj);
      if (!bValid) {
        std::stringstream strm;
        strm << "Badly formatted timestamp: " << sTimeString << std::endl;
        strm << __FILE__ << " (Line " << __LINE__ << ")";

        throw std::invalid_argument(strm.str());
      }
    }

    FromSeconds(stod(sTimeString));
    return *this;
  }

  /** Convert the timestamp to a floating point value.
   * @return The timestamp as a floating point numeric.
   */
  constexpr double ToSeconds() const {
    return std::chrono::duration<double>{dd}.count();
  }

  /** Set the timestamp from a floating point numeric.
   * @param dSeconds The timestamp in seconds.
   * @return A reference to this timestamp object.
   */
  CTimestampBase &FromSeconds(double const dSeconds) {
    if (dSeconds < 0.0) {
      std::stringstream strm;
      strm << "Negative time not supported: " << __FILE__ << " (Line "
           << __LINE__ << ")";
      throw std::out_of_range(strm.str());
    }

    auto dMaxSeconds =
        std::chrono::duration<double>{numeric_limits_max}.count();
    if (dSeconds > dMaxSeconds) {
      std::stringstream strm;
      strm << "Timestamp overflow: " << __FILE__ << " (Line " << __LINE__
           << ")";
      throw std::out_of_range(strm.str());
    }

    auto duration_s_double = std::chrono::duration<double>{dSeconds};
    dd = std::chrono::duration_cast<Duration>(
        duration_s_double); // std::chrono::duration_cast(), convert with
                            // trunctaion toward zero
    return *this;
  }
};

template <typename Rep = int64_t,
          typename Period = std::ratio<1, TIMESTAMP_TO_SECONDS_DIVISOR>>
constexpr std::ostream &operator<<(std::ostream &os,
                                   const CTimestampBase<Rep, Period> &t) {
  return os << t.ToString();
}

using CTimestamp = CTimestampBase<int64_t, std::micro>;

#include "../include/salawat.h"

auto main() -> i32
{
  Salawat ath("Syria", "Al-Tal", CalculationMethod::Makkah);
  ath.load();
  ath.print_timings();
  return 0;
}

#include <initializer_list>

// Launch a test case with different sizes for the Galois field.
template <typename Fn>
void
launch(std::initializer_list<std::uint8_t> sizes, Fn&& fn)
{
  for (const auto size : sizes)
  {
    fn(static_cast<std::uint8_t>(size));
  }
}

// Launch a test case with all different sizes for the Galois field.
template <typename Fn>
void
launch(Fn&& fn)
{
  launch({4,8,16,32}, std::forward<Fn>(fn));
}

// Launch a test case with different sizes for the Galois field.
template <typename Fn>
void
launch(Fn&& fn)
{
  for (const auto size : {4, 8, 16, 32})
  {
    fn(size);
  }
}

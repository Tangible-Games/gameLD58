#include "formatted_text_loader.hpp"

#include <gtest/gtest.h>

using namespace Symphony::Text;

TEST(FormattedText, SingleLine) {
  auto formatted_text = Symphony::Text::LoadFormattedText(
      "<style font=\"system_24.fnt\">Fps:</> <style "
      "font=\"system_50.fnt\"><sub variable=\"$fps_count\"></>",
      Symphony::Text::StyleWithAlignment(), {});
  ASSERT_EQ(formatted_text.paragraphs.size(), 1);
  ASSERT_EQ(formatted_text.paragraphs.back().style_runs.size(), 3);
  ASSERT_EQ(formatted_text.paragraphs.back().style_runs[0].text, "Fps:");
  ASSERT_EQ(formatted_text.paragraphs.back().style_runs[1].text, " ");
  ASSERT_EQ(formatted_text.paragraphs.back().style_runs[2].text, "60");
}

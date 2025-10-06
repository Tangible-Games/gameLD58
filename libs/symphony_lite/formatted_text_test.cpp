#include "formatted_text.hpp"

#include <gtest/gtest.h>

using namespace Symphony::Text;

TEST(FormattedText, SingleLine) {
  auto formatted_text = FormatText(
      "<style font=\"system_24.fnt\">Fps:</> <style "
      "font=\"system_50.fnt\"><sub variable=\"$fps_count\"></>",
      Style(), ParagraphParameters(), {{"fps_count", "60"}});
  ASSERT_TRUE(formatted_text.has_value());
  ASSERT_EQ((int)formatted_text->paragraphs.size(), 1);
  ASSERT_EQ((int)formatted_text->paragraphs[0].style_runs.size(), 3);
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[0].text, "Fps:");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[0].style.font,
            "system_24.fnt");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[1].text, " ");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[1].style.font, "");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[2].text, "60");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[2].style.font,
            "system_50.fnt");
}

TEST(FormattedText, OutputsLessThanSign) {
  auto formatted_text = FormatText("<<<style font=\"system_24.fnt\">Text</>",
                                   Style(), ParagraphParameters(), {});
  ASSERT_TRUE(formatted_text.has_value());
  ASSERT_EQ((int)formatted_text->paragraphs.size(), 1);
  ASSERT_EQ((int)formatted_text->paragraphs[0].style_runs.size(), 2);
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[0].text, "<");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[1].text, "Text");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[1].style.font,
            "system_24.fnt");
}

TEST(FormattedText, SpecifyStyle) {
  auto formatted_text = FormatText(
      "<style font=\"system_24.fnt\"><style color=\"red\"><style "
      "align=\"right\">Text</></></>",
      Style(), ParagraphParameters(), {});
  ASSERT_TRUE(formatted_text.has_value());
  ASSERT_EQ((int)formatted_text->paragraphs.size(), 1);
  ASSERT_EQ((int)formatted_text->paragraphs[0].style_runs.size(), 1);
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[0].text, "Text");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[0].style.font,
            "system_24.fnt");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[0].style.color,
            0xFFF00F13);
  ASSERT_EQ(formatted_text->paragraphs[0].paragraph_parameters.align,
            HorizontalAlignment::kRight);
}

TEST(FormattedText, LineBreakProducesNextParagraph) {
  auto formatted_text = FormatText(
      "<style font=\"system_24.fnt\">Fps:</> <style "
      "font=\"system_50.fnt\"><sub variable=\"$fps_count\"></>\n<style "
      "font=\"system_24.fnt\">Audio streams playing:</> <style "
      "font=\"system_50.fnt\"><sub variable=\"$audio_streams_playing\"></>",
      Style(), ParagraphParameters(),
      {{"fps_count", "60"}, {"audio_streams_playing", "16"}});
  ASSERT_TRUE(formatted_text.has_value());
  ASSERT_EQ((int)formatted_text->paragraphs.size(), 2);

  ASSERT_EQ((int)formatted_text->paragraphs[0].style_runs.size(), 3);
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[0].text, "Fps:");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[0].style.font,
            "system_24.fnt");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[1].text, " ");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[1].style.font, "");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[2].text, "60");
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[2].style.font,
            "system_50.fnt");

  ASSERT_EQ((int)formatted_text->paragraphs[1].style_runs.size(), 3);
  ASSERT_EQ(formatted_text->paragraphs[1].style_runs[0].text,
            "Audio streams playing:");
  ASSERT_EQ(formatted_text->paragraphs[1].style_runs[0].style.font,
            "system_24.fnt");
  ASSERT_EQ(formatted_text->paragraphs[1].style_runs[1].text, " ");
  ASSERT_EQ(formatted_text->paragraphs[1].style_runs[1].style.font, "");
  ASSERT_EQ(formatted_text->paragraphs[1].style_runs[2].text, "16");
  ASSERT_EQ(formatted_text->paragraphs[1].style_runs[2].style.font,
            "system_50.fnt");
}

TEST(FormattedText, AlignsParagraphs) {
  auto formatted_text = FormatText(
      "Left align\n<style align=\"right\">Right align\n<style "
      "align=\"center\">Center align</></>\nAgain left align",
      Style(), ParagraphParameters(), {});
  ASSERT_TRUE(formatted_text.has_value());
  ASSERT_EQ((int)formatted_text->paragraphs.size(), 4);
  ASSERT_EQ(formatted_text->paragraphs[0].paragraph_parameters.align,
            HorizontalAlignment::kLeft);
  ASSERT_EQ(formatted_text->paragraphs[0].style_runs[0].text, "Left align");
  ASSERT_EQ(formatted_text->paragraphs[1].paragraph_parameters.align,
            HorizontalAlignment::kRight);
  ASSERT_EQ(formatted_text->paragraphs[1].style_runs[0].text, "Right align");
  ASSERT_EQ(formatted_text->paragraphs[2].paragraph_parameters.align,
            HorizontalAlignment::kCenter);
  ASSERT_EQ(formatted_text->paragraphs[2].style_runs[0].text, "Center align");
  ASSERT_EQ(formatted_text->paragraphs[3].paragraph_parameters.align,
            HorizontalAlignment::kLeft);
  ASSERT_EQ(formatted_text->paragraphs[3].style_runs[0].text,
            "Again left align");
}

TEST(FormattedText, Stress) {
  auto formatted_text = FormatText(
      "<style font=\"system_30.fnt\" align=\"left\" "
      "wrapping=\"noclip\">Fps:</> <style font=\"system_50.fnt\"><sub "
      "variable=\"$fps_count\"></>\n<style font=\"system_30.fnt\" "
      "align=\"left\" wrapping=\"noclip\">Audio streams playing:</> <style "
      "font=\"system_50.fnt\"><sub "
      "variable=\"$audio_streams_playing\"></>\n",
      Style(), ParagraphParameters(),
      {{"fps_count", "60"}, {"audio_streams_playing", "500"}});
  ASSERT_TRUE(formatted_text.has_value());
}

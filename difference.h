#ifndef DIFFERENCE_H_081276A3
#define DIFFERENCE_H_081276A3

#include <random>
#include <string>
#include <Magick++.h>
#include <utility>
#include <verbly.h>
#include <stdexcept>
#include <mastodonpp/mastodonpp.hpp>
#include <memory>
#include "imagenet.h"

class difference {
public:

  difference(
    std::string configFile,
    std::mt19937& rng);

  void run() const;

private:

  verbly::word getPicturedNoun() const;

  std::pair<Magick::Image, Magick::Image>
    getImagesForNoun(verbly::word pictured) const;

  std::pair<verbly::word, verbly::word> getOppositeIdentifiers() const;

  Magick::Image composeImage(
    Magick::Image image1,
    verbly::word word1,
    Magick::Image image2,
    verbly::word word2) const;

  std::string generatePostText(verbly::word word1, verbly::word word2) const;

  void sendPost(std::string text, Magick::Image image) const;

  class could_not_get_images : public std::runtime_error {
  public:

    could_not_get_images() : std::runtime_error("Could not get images for noun")
    {
    }
  };

  std::mt19937& rng_;
  std::unique_ptr<verbly::database> database_;
  std::unique_ptr<mastodonpp::Instance> instance_;
  std::unique_ptr<mastodonpp::Connection> connection_;
  std::unique_ptr<imagenet> imagenet_;
  std::string fontfile_;
  std::string tempfile_;

};

#endif /* end of include guard: DIFFERENCE_H_081276A3 */

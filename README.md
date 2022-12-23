# difference
difference is a Mastodon bot that generates false dichotomies. It finds some type of thing, compares two different images of that type of thing, and then makes up a binary that explains why they are different. The bot parodies the black-and-white way that society tends to put things into dichotomies; for example, the gender binary. The original inspiration for this bot was the following tweet:

<blockquote class="twitter-tweet" data-lang="en"><p lang="en" dir="ltr">I label increasingly nonsensical images with ‘UI’ and ‘UX’ and hope they get used in serious presentations <a href="https://t.co/tDJgRp6CO5">pic.twitter.com/tDJgRp6CO5</a></p>&mdash; Sebastiaan de With (@sdw) <a href="https://twitter.com/sdw/status/709853249407361024?ref_src=twsrc%5Etfw">March 15, 2016</a></blockquote>

It tweets a new binary every hour. It uses [verbly](https://github.com/hatkirby/verbly) to pick nouns, pictures, and antonyms; [GraphicsMagick](http://www.graphicsmagick.org/) to create the output images; [YAMLcpp](https://github.com/jbeder/yaml-cpp) to read a configuration file; and [mastodonpp](https://github.com/tastytea/mastodonpp) to send the status updates to Mastodon.

The canonical difference handle is [@difference@beppo.online](https://beppo.online/@difference) (previously [@differencebot](https://twitter.com/differencebot)).

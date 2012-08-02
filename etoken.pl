#!/usr/bin/env perl

# Does the same thing as the etoken program. Relies on Text::Deva and its
# l_to_tokens() method, see https://github.com/glts/Text-Deva.
# The output is slightly different due to the limitations of this tokenizer.

use v5.12.1;
use strict;
use warnings;
use open ':encoding(UTF-8)';

use Text::Deva;

open my $def, '<', 'exampledef.txt' or die;
open my $in,  '<', 'example.txt'    or die;

my @defs = <$def>;
chomp @defs;

my $d = Text::Deva->new(
    C => do { my %c = map { $_ => 1 } @defs; \%c },
    V => {},
    F => {},
);

my @tokens;
while (<$in>) {
    push @tokens, @{ $d->l_to_tokens($_) };
}

for (@tokens) {
    say "Token: " . $_;
}

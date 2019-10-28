#!/usr/bin/ruby
# $Id: Compound2Basic.rb 6738 2018-01-14 16:42:43Z flaterco $
# For comparison purposes, reduce a congen_input Compound line to Basic
# coefficients for V and u.  Does not do node factors.
if ARGV.length != 0 && (ARGV[0] == "--help" || ARGV[0] == "-help")
  puts "Usage: Compound2Basic.rb [congen_input.txt] > output.txt"
  puts "Note:  Does not do node factors."
end

require "matrix"

# V portion:  T  s  h  p  p1 c
# u portion:  xi v vp dvpp Q R Qu
# Note that the last term in the u portion is inconsistent with the Basic
# input line format, which has node factor formula number in that position.
Comps = [
  Vector[1,-2,1,0,0,90,   2,-1,0,0,0,0,  0], # O1
  Vector[1,0,1,0,0,-90,   0,0,-1,0,0,0,  0], # K1
  Vector[1,0,-1,0,0,90,   0,0,0,0,0,0,   0], # P1
  Vector[2,-2,2,0,0,0,    2,-2,0,0,0,0,  0], # M2
  Vector[2,0,0,0,0,0,     0,0,0,0,0,0,   0], # S2
  Vector[2,-3,2,1,0,0,    2,-2,0,0,0,0,  0], # N2
  Vector[2,-1,2,-1,0,180, 2,-2,0,0,0,-1, 0], # L2
  Vector[2,0,2,0,0,0,     0,0,0,-1,0,0,  0], # K2
  Vector[1,-3,1,1,0,90,   2,-1,0,0,0,0,  0], # Q1
  Vector[2,-3,4,-1,0,0,   2,-2,0,0,0,0,  0], # NU2
  Vector[1,0,0,0,0,0,     0,0,0,0,0,0,   0], # S1
  Vector[1,-1,1,1,0,-90,  0,-1,0,0,0,0, -1], # M1-DUTCH
  Vector[2,-1,0,1,0,180,  2,-2,0,0,0,0,  0]  # LDA2
]
numcomps = Comps.length

STDERR.puts "WARNING"
STDERR.puts "Output is for comparison purposes only.  Compound2Basic.rb does not deal with"
STDERR.puts "node factors AT ALL."
puts "# WARNING"
puts "# This output is for comparison purposes only.  Compound2Basic.rb does not"
puts "# deal with node factors AT ALL."
puts "#"

ARGF.each_line {|origline|
  line = origline
  hash = line.index("#")
  unless hash.nil?
    line = (hash.zero? ? "" : line[0..(hash-1)])
  end
  fields = line.split
  if fields.length > 2 && fields[1] == "Compound"
    name = fields[0]
    coefs = fields[2..-1].map(&:to_i)
    if coefs.length > numcomps
      STDERR.puts origline
      STDERR.puts "Too many compound coefficients.  Is there a newer version of Congen?"
      raise "format error"
    elsif coefs.length < numcomps
      coefs.concat(Array.new(numcomps-coefs.length, 0))
    end
    result = Vector[0,0,0,0,0,0, 0,0,0,0,0,0,0]
    Comps.each_with_index {|comp,i| result += coefs[i]*comp}

    # Renormalize the constant angle
    result = result.to_a
    while result[5] <= -180 do result[5] += 360 end
    while result[5] > 180   do result[5] -= 360 end

    if !result[12].zero?
      puts "# WARNING"
      puts "# The next constituent has a coefficient of #{result[12]} for Qu."
      puts "# Basic has no field for that!"
    end
    printf "%-8s  Basic  %-18s %-16s  1\n",
          name,
          result[0..5].join(" "),
          result[6..11].join(" ")
  else
    puts origline
  end
}

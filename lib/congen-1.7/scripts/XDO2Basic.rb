#!/usr/bin/ruby
# 2018-01-08 19:27
# Translate XDO number from IHO list to the V part of the argument E in
# congen basic format.
#     name  Basic      T  s  h  p  p1 c  xi v vp dvpp Q R f#
# p1 = p' = pp

if ARGV.length != 1 or ARGV[0].length != 7
  puts "Usage: XDO2Basic.rb NNNNNNN"
  exit
end

def alph2num(cc)
  c = cc.upcase
  if c >= "R" and c <= "Z"
    c.ord - "Z".ord
  elsif c >= "A" and c <= "P"
    c.ord - "@".ord
  else
    raise "Illegal letter in XDO"
  end
end

def char2int0(cc)
  cc >= "0" && cc <= "9" ? cc.to_i : alph2num(cc)
end

def char2int5(cc)
  cc >= "0" && cc <= "9" ? cc.to_i-5 : alph2num(cc)
end

T = char2int0(ARGV[0][0])  # NO adjustment
vals = ARGV[0].each_char.map{|vc| char2int5(vc)}
s,h,p,N,p1 = vals[1..5]
c = vals[6] * 90

# Convert IHO/IOS usage of s, h, and c to SP98 usage.
s -= T
h += T
c = -c

if c <= -180
  c += 360
elsif c > 180
  c -= 360
end

# This is only the V part of the argument (E).
print "XX#{T}  Basic  #{T} #{s} #{h} #{p} #{p1} #{c}\n"

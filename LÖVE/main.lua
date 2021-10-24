--[[

]]

 Sfxr = require 'cppSfxr'

-- Load some default values for our rectangle.
function love.load()
    x, y, w, h = 20, 20, 60, 20
    Sfxr:create(Sfxr.Sound.EXPLOSION)
    source = love.audio.newSource(Sfxr:soundData())
end

-- Increase the size of the rectangle every frame.
function love.update(dt)
    w = w + 1
    h = h + 1
end

-- Draw a coloured rectangle.
function love.draw()
    -- In versions prior to 11.0, color component values are (0, 102, 102)
    love.graphics.setColor(0, 0.4, 0.4)
    love.graphics.rectangle("fill", x, y, w, h)
end

function love.keypressed( key, scancode, isrepeat )
  love.audio.play(source)
end

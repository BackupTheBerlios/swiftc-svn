#!/bin/bash
sed 's/(pos1.filename == pos2.filename/((pos1.filename == pos2.filename)/' <fe/position.hh >tmp && mv tmp fe/position.hh
sed 's/|| pos1.filename && pos2.filename && \*pos1.filename == \*pos2.filename)/|| (pos1.filename \&\& pos2.filename \&\& *pos1.filename == *pos2.filename))/' <fe/position.hh >tmp && mv tmp fe/position.hh


## Foot See 

Library that takes 3 photos and computes an ideal last.

### Installation as Module

Add this repo as a dependancy in your package.json
```
{
  "name": "Your App",
  "version": "9000.1.1",
  "dependencies": {
    "footsee": "git@github.com:graphicsforge/footsee.git"
  }
}
```
Then just `npm install`, and afterwards to grab the latest version `npm update`

### Usage as Module

```
const footsee = require('footsee');
const 3dprint = require('3d-printer');

footsee.build({"plantar": "./plantar.jpg", "medial": "./medial.jpg", "rulerless": "./rulerless.jpg", "side": "left"}, function(err, data) {
  3dprint(data.model);
})
```

### Installation as Server

Run `npm install` then `npm start`

### Usage as Server

`GET /` for usage
`POST /build` with the photos and what-not to download a model file

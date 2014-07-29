<!DOCTYPE html>
<html>
  <head>
    <title>Tank Designer</title>
    <link rel="stylesheet" href="style.css" type="text/css">
    <style type="text/css">
      #preview {
        float: right;
      }
      #sensors input {
        width: 5em;
      }
      #program textarea {
        width: 100%;
        min-height: 20em;
      }
      td {
        text-align: left;
      }
    </style>
    <script type="application/javascript" src="tanks.js"></script>
    <script type="application/javascript" src="designer.js"></script>
    <script type="application/javascript">
      window.onload = design;
    </script>
  </head>
  <body>
    <h1>Tank Designer</h1>
    <div id="preview"><canvas id="design"></canvas><p id="debug"></p></div>

    <p>
      Before you can get going with a tank, you need a password.  If you
      need a password, just ask one of the dirtbags.
    </p>

    <form action="upload.cgi" method="post">
      <fieldset id="metadata">
        <legend>Information</legend>
        <table>
          <tr>
            <td>Password:</td>
            <td><input name="token" type="password"></td>
          </tr>
          <tr>
            <td>Tank name:</td>
            <td><input name="name"></td>
          </tr>
          <tr>
            <td>Author:</td>
            <td><input name="author"> (eg. Joe Cool
              &lt;joe@cool.cc&gt;)</td>
          </tr>
          <tr>
            <td>Color:</td>
            <td><input name="color" type="color" value="#c0c0c0"
                       onchange="update();"> (eg. #c7e148)</td>
          </tr>
        </table>
      </fieldset>

      <fieldset id="sensors">
        <legend>Sensors</legend>
        <table>
          <thead>
            <tr>
              <td>#</td>
              <td>Range</td>
              <td>Angle</td>
              <td>Width</td>
              <td>Turret?</td>
            </tr>
          </thead>
          <tbody>
define(sensor,
`ifelse(`$1',`10',,`
            <tr>
              <td>$1</td>
              <td><input name="s$1r" type="number" min="0" max="100" onchange="update();"></td>
              <td><input name="s$1a" type="number" min="-360" max="360" onchange="update();"></td>
              <td><input name="s$1w" type="number" min="-360" max="360" onchange="update();"></td>
              <td><input name="s$1t" type="checkbox" onchange="update();"></td>
            </tr>
sensor(incr($1))')')
sensor(0)
          </tbody>
        </table>
      </fieldset>

      <fieldset id="program">
        <legend>Program</legend>
        <textarea name="program"></textarea>
      </fieldset>

      <input type="submit" value="Submit">
    </form>
include(nav.html.inc)
  </body>
</html>

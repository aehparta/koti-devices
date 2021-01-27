const baseurl = "";

$(document).ready(() => {
  $("h1").on("click", () => {
    $.get(baseurl + "switch/1/toggle").done(data => {
      console.log(data);
    });
  });
});

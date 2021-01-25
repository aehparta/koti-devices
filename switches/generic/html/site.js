const baseurl = "";

$(document).ready(() => {
  $("h1").on("click", () => {
    $.get(baseurl + "switch/1");
  });
});

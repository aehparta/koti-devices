const baseurl = "";

$(document).ready(() => {
  $("body").on("click", ".switch", function () {
    const sw = $(this).attr("switch");
    $.ajax(baseurl + "switch/" + sw + "/toggle", {
      type: "PUT",
      data: "toggle",
    }).done((data) => {
      $(this).removeClass("on");
      $(this).removeClass("off");
      if (data === "on") {
        $(this).addClass("on");
      } else if (data === "off") {
        $(this).addClass("off");
      }
    });
  });
});

const baseurl = "";
let switch_count = 2;

const switch_update = (id, state) => {
  const sw = $(".switch[switch=" + id + "]");
  sw.removeClass("on");
  sw.removeClass("off");
  state ? sw.addClass("on") : sw.addClass("off");
};

$(document).ready(() => {
  $("body").on("click", ".switch", function () {
    const id = $(this).attr("switch");
    $.ajax(baseurl + "switch/" + id, {
      type: "PUT",
      data: "toggle",
    }).done((data) => {
      switch_update(id, data === "on");
    });
  });

  for (let id = 0; id < switch_count; id++) {
    $.get(baseurl + "switch/" + id, (data) => {
      switch_update(id, data === "on");
    });
  }
});
